#include "serv.h"
#include "motors.h"
#include "controldeVol.hpp"
#include "capteur.hpp"

int main (int argc, char *argv[]){

	char verbose = 0;
	if (argc > 1) {
		if (strcmp(argv[1], "--verbose") == 0) {
			printf("verbose MODE select\n");
			verbose = 1;
		}
	}else{
		printf("add    --verbose   for verbose mode\n");
	}

	PMutex * PmutexRemoteConnect = (PMutex *) malloc(sizeof(PMutex));
	if(PmutexRemoteConnect==NULL){
		perror("MALLOC FAIL : PmutexRemoteConnect\n");
		return EXIT_FAILURE;
	}
	init_PMutex(PmutexRemoteConnect);

	PMutex * PmutexDataControler = (PMutex *) malloc(sizeof(PMutex));
	if (PmutexDataControler == NULL) {
		perror("MALLOC FAIL : PmutexDataControler\n");
		return EXIT_FAILURE;
	}
	init_PMutex(PmutexDataControler);

	char myIP[64];
	getIP(myIP);

	DataController * dataControl =(DataController *) malloc(sizeof(DataController));
	if (dataControl == NULL) {
		perror("MALLOC FAIL : dataControl\n");
		return EXIT_FAILURE;
	}

	dataControl->pmutex=PmutexDataControler;
	dataControl->flag=2;

	args_SERVER * argServ =(args_SERVER *) malloc(sizeof(args_SERVER));
	if (argServ == NULL) {
		perror("MALLOC FAIL : argServ\n");
		return EXIT_FAILURE;
	}
	argServ->pmutexRemoteConnect = PmutexRemoteConnect;
	argServ->dataController = dataControl;
	argServ->verbose=verbose;

	MotorsAll * motorsAll =(MotorsAll *) malloc(sizeof(MotorsAll));
	if (motorsAll == NULL) {
		perror("MALLOC FAIL : motorsAll\n");
		return EXIT_FAILURE;
	}
	motorsAll->bool_arret_moteur =(volatile int *) malloc(sizeof(int));

	if (motorsAll->bool_arret_moteur == NULL) {
		perror("MALLOC FAIL : motorsAll->bool_arret_moteur\n");
		return EXIT_FAILURE;
	}
	volatile int arret=0;
	*(motorsAll->bool_arret_moteur)= arret;

	if(init_Value_motors(motorsAll)==0){
		return EXIT_FAILURE;
	}

	args_CONTROLDEVOL * argCONTROLVOL =(args_CONTROLDEVOL *) malloc(sizeof(args_CONTROLDEVOL));
	if (argCONTROLVOL == NULL) {
		perror("MALLOC FAIL : argCONTROLVOL\n");
		return EXIT_FAILURE;
	}
	argCONTROLVOL->dataController=dataControl;
	argCONTROLVOL->motorsAll=motorsAll;
	argCONTROLVOL->verbose=verbose;


	#ifdef __arm__
	RTIMU *imu;
	imu = capteurInit();

	if(imu==NULL){
		perror("NEW FAIL : RTIMU ->imu\n");
		return EXIT_FAILURE;
	}else{
		if(verbose){printf("THREAD MAIN : CAPTEUR INIT SUCCES\n");}
		argCONTROLVOL->imu=imu;
	}
	#endif

	pthread_t threadServer;
	pthread_t threadControlerVOL;


	pthread_mutex_lock(&PmutexRemoteConnect->mutex);

	if (pthread_create(&threadServer, NULL, thread_UDP_SERVER, argServ)!=0) {
		perror("THREAD MAIN : pthread_create SERVER\n");
		return EXIT_FAILURE;
	}

	pthread_cond_wait(&PmutexRemoteConnect->condition, &PmutexRemoteConnect->mutex);

	pthread_mutex_unlock(&PmutexRemoteConnect->mutex);


	if (pthread_create(&threadControlerVOL, NULL, startCONTROLVOL, argCONTROLVOL)!=0) {
		perror("THREAD MAIN : pthread_create PID\n");
		return EXIT_FAILURE;
	}

	init_threads_motors(motorsAll,verbose);//start the 4 threads et ne rends pas la main

	//TODO test fermeture NORMAL des PTHREADS moteurs

	int * returnValue;

	if (pthread_join(threadServer,(void**) &returnValue)!=0){
		perror("THREAD MAIN : pthread_join SERVER\n");
		return EXIT_FAILURE;
	}
	if (pthread_join(threadControlerVOL, (void**) &returnValue)!=0){
		perror("THREAD MAIN : pthread_join PID\n");
		return EXIT_FAILURE;
	}


	clean_args_SERVER(argServ);
	clean_args_CONTROLDEVOL(argCONTROLVOL);

	if(verbose){printf("THREAD MAIN : END\n");}
	return EXIT_SUCCESS;
}
