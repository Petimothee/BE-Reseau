#include <mictcp.h>
#include <api/mictcp_core.h>
#include <pthread.h>
#define SIZE 10

mic_tcp_sock sock;
mic_tcp_sock_addr addr_distant;
unsigned int PE = 0;
unsigned int PA = 0; 
int threshold = 20; //threshold de pertes en pourcentage
int tabCirc[SIZE];
static int i=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void error(char* error_message, int line){
    fprintf(stderr, "%s at line %d\n", error_message, line);
    exit(EXIT_SUCCESS);
}
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
    int result = -1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    result = initialize_components(sm); /* Appel obligatoire */
    set_loss_rate(0);
    sock.fd = 0;
    return sock.fd;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   if (socket != sock.fd) {
        printf("Erreur socket");
        return -1;
    }
    sock.addr = addr;
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) //cote server, donc asynchrone (-> usage des mutex)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    //wait for syn
    if (sock.fd == socket){
        sock.state = WAIT_FOR_SYN;
        if (pthread_mutex_lock(&mutex))
            error("Erreur lock mutex",__LINE__);
        
        while (sock.state==WAIT_FOR_SYN){
            if (pthread_cond_wait(&cond,&mutex)) //blocage des threads en attendant que la condition soit satisfaite
                error("Erreur cond_wait SYN",__LINE__);
        }
        if (pthread_mutex_unlock(&mutex))
            error("Erreur unlock mutex",__LINE__);
    }

    //construction du SYNACK
    mic_tcp_pdu SYNACK = {0};
    SYNACK.header.source_port = sock.addr.port;
    SYNACK.header.dest_port = addr_distant.port;
    SYNACK.header.ack = 1;
    SYNACK.header.syn = 1;

    //envoie du SYNACK
    if(IP_send(SYNACK, *addr) < 0)
        error("Erreur IP Send", __LINE__);
    
    printf("Envoi du SYNACK avec syn = %d et ack = %d\n", SYNACK.header.syn, SYNACK.header.ack);

    //wait for ack
    if (sock.fd == socket){
        sock.state = WAIT_FOR_ACK;
        if (pthread_mutex_lock(&mutex))
            error("Erreur lock mutex",__LINE__);
        
        while (sock.state==WAIT_FOR_ACK){
            if (pthread_cond_wait(&cond,&mutex)) //blocage des threads en attendant que la condition soit satisfaite
                error("Erreur cond_wait ACK",__LINE__);
        }
        if (pthread_mutex_unlock(&mutex))
            error("Erreur unlock mutex",__LINE__);
    }
    printf("ACK bien recu cote application!\n");

    //sock.state = ESTABLISHED;
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    //sock.state = IDLE;
    int nb_essais = 0;
    int nb_max_essais = 10;

    //Construction SYN
    mic_tcp_pdu SYN = {0};
    SYN.header.source_port = sock.addr.port;
    SYN.header.dest_port = addr_distant.port;
    SYN.header.syn=1;
    
    //Envoi du SYN
    if(IP_send(SYN, addr) < 0)
        error("Erreur IP Send", __LINE__);
    nb_essais++;

    //wait for syn ack
    mic_tcp_pdu SYNACK = {0};
    int recep_SYNACK = 0;
    int synack_size;
    sock.state = WAIT_FOR_SYNACK;
    while (!recep_SYNACK && (nb_essais < nb_max_essais)) {
        printf("On rentre dans le wait_for_synack\n");
        if ((synack_size = IP_recv(&SYNACK, &addr_distant, 100)) < 0) { //timeout
            //retransmission du SYN
            if(IP_send(SYN, addr) < 0) 
                error("error ip-send",__LINE__);
            sleep(1);
            nb_essais++;
        }
        else {
            if ((SYNACK.header.syn != 1) || (SYNACK.header.ack != 1)){ //c'est pas un synack
                printf("\nErreur SYNACK avec syn = %d et ack = %d\n", SYNACK.header.syn, SYNACK.header.ack);
                if(IP_send(SYN, addr) < 0) 
                    error("error ip-send",__LINE__);
                sleep(1);
                nb_essais++;
            }
            else{   //ca marche bien
                printf("SYNACK bien recu\n");
                recep_SYNACK = 1;
                //Construction ACK
                mic_tcp_pdu ACK = {0};
                ACK.header.source_port = sock.addr.port;
                ACK.header.dest_port = addr_distant.port;
                ACK.header.ack=1;
                //Envoi de l'ACK
                if(IP_send(ACK, addr) < 0) 
                    error("error ip-send",__LINE__);

                printf("ACK bien envoyé avec ack=%d et syn=%d\n", ACK.header.ack, ACK.header.syn);
                sock.state = ESTABLISHED;
                break;
            }
        }
    }
    if((recep_SYNACK == 0) && (nb_essais >= nb_max_essais)){
        printf("Trop d'essais d'envois du cote client avant de recevoir le synack\n");
        sock.state = IDLE;
        return -1;
    }
    
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 * Gere la gestion de la fiabilite partielle
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
   
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu pdu = {0};
    int nombre_essais = 0;
    int nombre_essais_max = 10;
    //test si mic_sock = a socket
    if (mic_sock != sock.fd) {
        printf("Erreur socket");
        return -1;
    }

    //creer PDU
    pdu.header.source_port=sock.addr.port; 
    pdu.header.dest_port=addr_distant.port; 
    pdu.header.seq_num=PE;
    pdu.payload.data=mesg;
    pdu.payload.size=mesg_size;

    //envoi
    int sent_size = 0;
    if ((sent_size = IP_send(pdu, addr_distant)) == -1) {
        error("Erreur IP_send", __LINE__);
    }
    PE = (PE+1)%2;
    nombre_essais++;

    //on va creer une liste circulaire avec un 1 ou un 0 dans chaque case. 1 si perte, 0 sinon. S'il y a plus de 1 que le threshold (variable globale) dans le tableau circulaire, on renvoie le dernier paquet
    //gestion de fiabilite partielle
    //wait for ack
    int active=1;
    int ack1=-1;
    mic_tcp_pdu ack = {0}; 
    //gestion de fiabilite partielle
    int compteur1=0;
    while (active && (nombre_essais < nombre_essais_max)) {
        printf("On rentre dans le wait_for_ack\n");
        if ((ack1 = IP_recv(&ack, &addr_distant, 100)) < 0) { //timeout
            tabCirc[i] = 1;
            for(int j=0; j<SIZE; j++){            //on parcourt le tableau pour voir si on doit renvoyer ou non le paquet
                compteur1 = compteur1 + tabCirc[j];
            }
            if(compteur1>(threshold/100)*SIZE){     //s'il y a plus d'erreur que le threshold, on renvoi le paquet
                if(IP_send(pdu, addr_distant) < 0)  
                    error("error ip-send",__LINE__);
                nombre_essais++;
            }
            i=(i+1)%SIZE; //on reinitialize l'indice a 0 s'il depasse la taille du tableau (liste circulaire version sans pointeurs)
        }
        else { //réception ack
            if ((ack.header.ack_num != PE) || (ack.header.ack != 1)){ //c'est pas le bon
                printf("\nErreur pdu->header.ack_num != PE\n");
                tabCirc[i] = 1;
                for(int k=0; k<SIZE; k++){
                    compteur1 = compteur1 + tabCirc[k];
                }
                if(compteur1>(threshold/100)*SIZE){
                    if(IP_send(pdu, addr_distant) < 0) 
                        error("error ip-send",__LINE__);
                    nombre_essais++;
                }
                i=(i+1)%SIZE;
            }
            else{   //ca marche bien
                printf("Accuse de reception bien recu\n");
                tabCirc[i] = 0;
                i=(i+1)%SIZE;                //pas besoin de parcourir le tableau pq on a pas d'erreur sur i
                active = -1;
                break;
            }
        }
    }

    if (ack1<0)
        error("erreur ack", __LINE__);
    
    printf("seq num : %d\n", pdu.header.seq_num);
    return sent_size;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    mic_tcp_payload app_buff = {
        .data = mesg,
        .size = max_mesg_size
    };
    
    //on recupere du buffer
    return app_buffer_get(app_buff);
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr){
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    //il faut maintenant rajouter la recuperation du syn et de l'ack du cote serveur car il est asynchrone. Ainsi on va utiliser la variable de condition et broadcast quand on recoit un pdu avec syn=1 et ack=1 respectivement, ce qui enleve le blocage de threads imposé dans l'accept().

    //recuperation du syn
    if(sock.state == WAIT_FOR_SYN){
        if((pdu.header.syn == 1) && (pdu.header.ack != 1)){
            if(pthread_cond_broadcast(&cond) != 0)
                error("Erreur broadcast cond", __LINE__);
            sock.state = WAIT_FOR_ACK;
            if(pthread_mutex_unlock(&mutex) != 0)
                error("Erreur mutex unlock", __LINE__);
        }
    }
    //recuperation de l'ack
    else if(sock.state == WAIT_FOR_ACK){
        if((pdu.header.ack == 1) && (pdu.header.syn != 1)){
            if(pthread_cond_broadcast(&cond) != 0)
                error("Erreur broadcast cond", __LINE__);
            sock.state = ESTABLISHED; //wait_for_ack est la derniere etape avant d'etablir la connexion
            if(pthread_mutex_unlock(&mutex) != 0)
                error("Erreur mutex unlock", __LINE__);
            printf("ACK bien recu cote serveur!\n");

        }
    }

    //recuperation de data
    else{
        printf("seq num : %d \n", pdu.header.seq_num); 
        
        //insertion dans buffer si son seq num == PA
        if (pdu.header.seq_num == PA) {
            printf("seq num == PA\n");
            app_buffer_put(pdu.payload);
            //maj du PA
            PA = (PA +1) % 2; 
            //construction ack
            mic_tcp_pdu ack={0}; 
            ack.header.source_port = sock.addr.port;
            ack.header.dest_port = addr.port;
            ack.header.ack_num = PA; 
            ack.header.ack = 1; 
            ack.header.ack_num = PA; 
            //envoi de l'ack
            if(IP_send(ack, addr) < 0)
                error("problem ack", __LINE__);
        }
    }
}

//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