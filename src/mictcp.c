#include <mictcp.h>
#include <api/mictcp_core.h>
#define taille_tab 10

//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

mic_tcp_sock sock;
int index_envoie = 0;
int tab[taille_tab];
int taux_pertes = 20;

void init_tab(){
    for (int j=0; j<taille_tab; j++) {
    tab[j] = 0;
    }
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
   set_loss_rate(taux_pertes);
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
 /* Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    // printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    // sock.state = IDLE;
    // mic_tcp_pdu pdu_SYN;
    // //on construit un pdu
    // pdu_SYN = creer_PDU_vide(socket);
    // //WAIT FOR SYN
    // if (wait_for_syn(&pdu_SYN, addr) <= 0){
    //     printf("Erreur WAIT_FOR_SYN\n");
    //     exit(-1);
    // }
    // //on construit un pdu SYNACK
    // mic_tcp_pdu pdu_SYNACK;
    // pdu_SYNACK = creer_PDU_vide(socket);
    // pdu_SYNACK.header.ack=1;
    // pdu_SYNACK.header.syn=1;
    // //Envoi du SYN_ACK 
    // int sent_size;
    // if ((sent_size = IP_send(pdu_SYNACK, *addr)) == -1) {
    //         printf("Erreur IP_send\n");
    //     exit(-1);
    // }
    // //WAIT FOR ACK
    // mic_tcp_pdu pdu_ACK;
    // pdu_ACK = creer_PDU_vide(socket);
    // if (wait_for_ack2(&pdu_ACK, addr) <= 0){
    //     printf("Erreur WAIT_FOR_ACK\n");
    //     exit(-1);
    // }
    //index_tab++;
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
    init_tab();
    // printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    // sock.state = IDLE;
    // //Construire SYN
    // mic_tcp_pdu pdu_SYN;
    // pdu_SYN = creer_PDU_vide(socket);
    // pdu_SYN.header.syn = 1;
    // //Envoyer SYN
    // int sent_size;
    // if ((sent_size = IP_send(pdu_SYN, addr)) == -1) {
    //     printf("Erreur IP_send du SYN");
    //     exit(-1);taille_tab 
    //index_tab++;_vide(socket);
    // pdu_ACK.header.ack = 1;
    // //Envoyer ACK
    // if ((sent_size = IP_send(pdu_ACK, addr)) == -1) {
    //     printf("Erreur IP_send du ACK");
    //     exit(-1);
    // }
    // sock.state = ESTABLISHED;
    // return 0;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock.state = ESTABLISHED;
    return 0;
}
/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    int compteur_erreur = 0;
    unsigned int PE = 0;
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu pdu;
    
    //test si mic_sock = a socket
    if (mic_sock != sock.fd) {
        printf("Erreur socket");
        return -1;
    }

    //header
    
    //activation int compteur_erreur = 0;

    //payload
    char *data = mesg;
    int size = mesg_size;

    //creer PDU
    pdu = creer_PDU2(mic_sock, data, size, PE);
    
    //envoi
    int sent_size;
    if ((sent_size = IP_send(pdu, sock.addr)) == -1) {
        printf("Erreur IP_send");
        exit(-1);
    }
    

    int size_ack;
    if ((size_ack = wait_for_ack(&pdu, &sock.addr))<0){
        printf("Erreur ack\n");
        return sent_size = 0;
    }
    int size_ack2;
    if (size_ack==-1){
        size_ack2=1;
    }
    tab[index_envoie%10]=size_ack2;
    printf("\n");
    for (int i=0;i<10;i++){
        if (tab[i]==-1) {
            compteur_erreur++;
            printf("Tableau : %d\n", tab[i]);
        }

    }
    if ((compteur_erreur/taille_tab)*100>20){
        if ((sent_size = IP_send(pdu, sock.addr)) == -1) {
        printf("Erreur IP_send taux de pertes\n");
        exit(-1);
        }
    }
   
    PE = (PE+1)%2;
    index_envoie++;
    printf("compteur : %d\n", compteur_erreur);
    printf("index envoie : %d\n", index_envoie);
    return sent_size;
}

//creer_pdu sans reprise de pertes
mic_tcp_pdu creer_PDU1(int dest_port, char *data, int size) {
    mic_tcp_pdu *pdu= malloc(sizeof(mic_tcp_pdu));
    mic_tcp_header *header = malloc(sizeof(mic_tcp_header)); /* entête du PDU */
    mic_tcp_payload *payload = malloc(sizeof(mic_tcp_payload)); 
    header->source_port=80;
    header->dest_port=(unsigned short)dest_port;
    header->seq_num=0;
    header->ack_num=0;
    header->syn=0;
    header->ack=printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock.state = ESTABLISHED;
    return *pdu;
}

//creer_pdu avec mecanisme de reprise de pertes
mic_tcp_pdu creer_PDU2(int dest_port, char *data, int size,unsigned int PE) {
    mic_tcp_pdu *pdu= malloc(sizeof(mic_tcp_pdu));
    mic_tcp_header *header = malloc(sizeof(mic_tcp_header)); /* entête du PDU */
    mic_tcp_payload *payload = malloc(sizeof(mic_tcp_payload)); 
    header->source_port=80;
    header->dest_port=(unsigned short)dest_port;
    header->seq_num=PE;
    header->ack_num=0;
    header->syn=0;
    header->ack=0;
    header->fin=0;
    payload->data = data;
    payload->size = size;
    pdu->header=(mic_tcp_header) *header;
    pdu->payload=(mic_tcp_payload) *payload;
    return *pdu;
}

/**
//on creer un pdu vide
mic_tcp_pdu creer_PDU_vide(int dest_port) {
    mic_tcp_pdu *pdu= malloc(sizeof(mic_tcp_pdu));
    mic_tcp_header *header = malloc(sizeof(mic_tcp_header)); /* entête du PDU
    mic_tcp_payload *payload = malloc(sizeof(mic_tcp_payload)); 
    header->source_port=80;
    header->dest_port=(unsigned short)dest_port;
    header->seq_num=0;
    header->ack_num=0;
    header->syn=0;
    header->ack=0;
    header->fin=0;
    payload->data = "test";
    payload->size = 0;
    pdu->header=(mic_tcp_header) *header;
    pdu->payload=(mic_tcp_payload) *payload;
    return *pdu;
}
**/
//WAIT_FOR_ACK() (avec le seq num != PE)
int wait_for_ack(mic_tcp_pdu *pdu, mic_tcp_sock_addr * addr) {
    int active=1;
    int ack1 = -1;
    unsigned int PE = pdu->header.seq_num;
    while (active) {
        if ((ack1 = IP_recv(pdu, addr, 5)) <= 0) {
            active = -1;
        }
        if (pdu->header.seq_num != PE){ //c'est pas le bon
            active = -1;
            ack1 =-1;
            printf("\nErreur pdu->header.seq_num != PE\n");
        }
        else{
            active = -1; //ca marche bien (askip)
        }
    }
    return ack1;
}
/*
//WAIT_FOR_ACK() (avec l'ack != 1)
int wait_for_ack2(mic_tcp_pdu *pdu, mic_tcp_sock_addr * addr) {
    int active=1;
    int ack2;
    while (active) {
        if ((ack2 = IP_recv(pdu, addr, 5)) < 0) {
            active = -1;
            printf("\nErreur ACK (cote recv)\n");
        }
        if (pdu->header.ack != 1){ //ca marche pas
            active = -1;
            ack2 =0;for (int j=0; j<taille_tab; j++) {
    tab[j] = 0;
}
    }
    return ack2;
}
*/
/*
//WAIT_FOR_SYN() si egal a 0, ca marche pas. Si egal a 1, ca marche
int wait_for_syn(mic_tcp_pdu *pdu, mic_tcp_sock_addr * addr) {
    int active=1;
    int syn1;
    while (active) {
        if ((syn1 = IP_recv(pdu, addr, 5)) < 0) {
            printf("\nErreur SYN (cote recv)\n");
            active = -1;
        }
        if (pdu->header.syn != 1){ //c'est pas le bon
            active = -1;
            syn1 = 0;
            printf("\nErreur SYN\n");
        }
        else{
            active = -1; //ca marche bien (askip)
        }
    }
    return syn1;
}else
    int ack1;
    int test;
    while (active) {
        if ((test = IP_recv(pdu, addr, 5)) < 0) {
            printf("\nErreur SYNACK (cote recv)\n");
            active = -1;
        }
        if (pdu->header.syn != 1 || pdu->header.ack != 1){ //ca marche pas
            active = -1;
            syn1 = 0; ack1 =-1;
        }
        else{
            active = -1; //ca marche bien (askip)
        }
    }
    return (syn1 && ack1 && test); //normalement si tout marche c'est egal a 1
}
*/

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
    return -1;
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
    app_buffer_put(pdu.payload);
    int sent_size;
    //on envoie le ack
    

    // Recevoir un segemnt 


    // Vérifier l'égalité des numéros de séquences
    int num_seq = 0; // num seq de base

    if (num_seq == pdu.header.seq_num) {
        // donner à l'app

        if ((sent_size = IP_send(pdu, addr)) == -1) {
            printf("Erreur IP_send dans process_received");
            exit(-1);
        }
        printf("\n Process received sucessful\n");
        num_seq=(num_seq+1)%2;
        // incrémennter num_seq
    } 
    else {
        printf("\nErreur num_seq == pdu.header.seq_num\n");

        //envoie à l'emetteur que j'ai pas recu le boon segment
    }

}
