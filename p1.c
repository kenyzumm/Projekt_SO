/* 

#define SIG2 = 2 // sygnał od P2 


void signalhandler(int sig){
        printf("Otrzymałem sygnał: %d", sig);
   } ???
   */




pid1 = fork();
    if(pid1 == 0) {
        // Kod P1 - konsument

        signal(SIG2, sig_handler); // p1  zeka na sygnal od p2 
        while(1) {
         
            P.sem_num = 0; semop(semid, &P, 1);                //p1 czeka na semafor
            printf("P1 odczytuje liczbę: %d\n", *shared_data);  //p1 czyta liczbe znakow z wspolniej pamieci i ja wypisuje
            V.sem_num = 0; semop(semid, &V, 1);   //ustawia semafor na 0(zwalnia go), p1 sie blokuje
            
        }
        exit(0);
    }
