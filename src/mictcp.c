#include <mictcp.h>
#include <api/mictcp_core.h>
#define SIZE 10

mic_tcp_sock sock;
unsigned int PE = 0;
unsigned int PA = 0; 
int threshold = 20; //threshold de pertes en pourcentage
int tabCirc[SIZE];
static int i=0;
int start=0;

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
    set_loss_rate(50);
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
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock.state = ESTABLISHED;
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock.state = ESTABLISHED;
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

    //test si mic_sock = a socket
    if (mic_sock != sock.fd) {
        printf("Erreur socket");
        return -1;
    }

    //header
    
    //activation

    //payload
    char *data = mesg;
    int size = mesg_size;

    //creer PDU
    pdu.header.source_port=80;
    pdu.header.dest_port=(unsigned short)mic_sock;
    pdu.header.seq_num=PE;
    pdu.payload.data = data;
    pdu.payload.size = size;

    //envoi
    int sent_size = 0;
    if ((sent_size = IP_send(pdu, sock.addr)) == -1) {
        printf("Erreur IP_send");
        return(-1);
    }
    PE = (PE+1)%2;

    //on va creer une liste circulaire avec un 1 ou un 0 dans chaque case. 1 si perte, 0 sinon. S'il y a plus de 1 que le threshold (variable globale) dans le tableau circulaire, on renvoie le dernier paquet
    //gestion de fiabilite partielle
    //wait for ack
    int active=1;
    int ack1=-1;
    mic_tcp_pdu ack = {0}; 
    //gestion de fiabilite partielle
    int compteur1=0;
    while (active) {
        printf("On rentre dans le wait_for_ack\n");
        if ((ack1 = IP_recv(&ack, &sock.addr, 100)) < 0) { //timeout
            tabCirc[i] = 1;
            i=(i+1)%SIZE; //on reinitialize l'indice a 0 si il depasse la taille du tableau (liste circulaire version sans pointeurs)
            //on parcourt le tableau pour voir si on doit renvoyer ou non le paquet
            for(int j=0; j<SIZE; j++){
                compteur1 = compteur1 + tabCirc[j];
            }
            if(compteur1>(threshold/100)*size){
                if(IP_send(pdu, sock.addr) < 0) // à intégrer un nombre max de retransmission  
                    error("error ip-send",__LINE__);
            }
        }
        else { //réception ack
            if ((ack.header.ack_num != PE) || (ack.header.ack != 1)){ //c'est pas le bon
                printf("\nErreur pdu->header.ack_num != PE\n");
                tabCirc[i] = 1;
                i=(i+1)%SIZE;
                //on parcourt le tableau pour voir si on doit renvoyer ou non le paquet
                for(int k=0; k<SIZE; k++){
                    compteur1 = compteur1 + tabCirc[k];
                }
                if(compteur1>(threshold/100)*SIZE){
                    if(IP_send(pdu, sock.addr) < 0) // à intégrer un nombre max de retransmission  
                        error("error ip-send",__LINE__);
                }
            }
            else{   //ca marche bien (askip)
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
    
    //PE = (PE+1)%2;
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
    /*
    mic_tcp_payload p;
    p.data= malloc(max_mesg_size);
    p.size = app_buffer_get(p);
    memcpy(mesg, p.data, max_mesg_size);
    free(p.data);
    return p.size; */
       mic_tcp_payload app_buff = {
        .data = mesg,
        .size = max_mesg_size
    };
    //app_buff.data = mesg;
    //app_buff.size = max_mesg_size;
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
    //desencapsuler PDU
    /*mic_tcp_header header_pdu = pdu.header;
    mic_tcp_payload payload_pdu = pdu.paypload ;     
    char* msg= payload_pdu.data;*/
    // (on insere le pdu dans le buffer de réception car on est en  asynchrone coté réception)
   
    printf("pdu seq num %d \n", pdu.header.seq_num); 
    mic_tcp_pdu ack; 
    ack.header.ack_num = PA; 
    ack.header.ack = 1; 
    ack.payload.size = 0; 
    ack.payload.data = NULL;
    if (pdu.header.seq_num == PA) {
        printf("seq num == PA\n");
        app_buffer_put(pdu.payload);
        PA = (PA +1) % 2; 
        ack.header.ack_num = PA; 
    }
    if(IP_send(ack, addr) < 0)
        error("problem ack", __LINE__);
}


//gbenalay@insa-toulouse.fr
//ghada gharbi
//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