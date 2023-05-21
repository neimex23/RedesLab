/*

Redes de Computadoras - Curso 1er Semestre 2021
Tecnologo en Informatica FIng - CETP

Entrega 2  - Programacion con Sockets
Sistema basico de Mensajeria

Integrantes:

	*Anna Torres 5.255.652-8
	*Santiago Gamarra 4.917.301-2



*/

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <ctype.h>
#include <iostream>
#include <errno.h>
#include <netinet/in.h>

#define MAX_LARGO_MENSAJE 255
#define MAX_NOMBRE 25

using namespace std;

void manejadorSenhales (int signal) {

	if (signal == SIGINT){
		cout << "\33[46m\33[31m[" << getpid() << "]" << " SIGINT CTRL+C recibido\33[00m\n";
	}
	if (signal == SIGTERM){
		cout << "\33[46m\33[31m[" << getpid() << "]" << " SIGTERM Terminacion de programa\33[00m\n";
	}
	if (signal == SIGSEGV){
		cout << "\33[46m\33[31m[" << getpid() << "]" << " SIGSEGV violacion de segmento\33[00m\n";
	}
	if (signal == SIGCHLD){
		cout << "\33[46m\33[31m[" << "]" << " SIGCHLD \33[00m\n";
	}
	if (signal == SIGPIPE){
		cout << "\33[46m\33[31m[" << getpid() << "]" << " SIGPIPE \33[00m\n";
	}
	if (signal == SIGKILL){
		cout << "\33[46m\33[31m[" << getpid() << "]" << " SIGKILL \33[00m\n";
	}
	exit(1);
}

void resetString (char * & s) {

	s[0] = '\0';
}

char * agregarCero (char * cad, int num) {

	char * aux = new char[25];
	resetString(aux);
	strcat(aux,"0");
	sprintf(cad,"%d",num);
	if(num<10){
		strcat(aux,cad);
		return aux;
	}
	else{
		delete[] aux;
		return cad;
	}
}

char * getTiempo () {

	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	int hh = timeinfo -> tm_hour;
	int mm = timeinfo -> tm_min;
	int ss = timeinfo -> tm_sec;
	int DD = timeinfo -> tm_mday;
	int MM = (timeinfo -> tm_mon) + 1; //xq enero = 0
	int YYYY = (timeinfo -> tm_year) + 1900 ; //xq el año es desde 1900
	char * s = new char[19];
	resetString(s);
	char * cad = new char[25];
	resetString(cad);
	//chequeo si alguna es menor q 10 para concatenarle un '0'
	strcat(s,"[");
	cad = agregarCero(cad,YYYY);
	strcat(s,cad);
	strcat(s,".");
	cad = agregarCero(cad,MM);
	strcat(s,cad);
	strcat(s,".");
	cad = agregarCero(cad,DD);
	strcat(s,cad);
	strcat(s," ");
	cad = agregarCero(cad,hh);
	strcat(s,cad);
	strcat(s,":");
	cad = agregarCero(cad,mm);
	strcat(s,cad);
	strcat(s,":");
	cad = agregarCero(cad,ss);
	strcat(s,cad);
	strcat(s,"]");
	return s;
}

void md5 (char contrasena[]) {

	int num_characters = 0;
	int i = 0;

	ifstream myfile ("md5.txt");

	if (myfile.is_open()) {

		while ( !myfile.eof()) {

	        myfile >> contrasena[i];
	        i++;
	        num_characters ++;
	    }
	}
}


void getPathArchivo(string *strPathArchivo, char mensaje[]){

	int posicion = 0;
	bool copiarPath = false;

	while (posicion < MAX_LARGO_MENSAJE - 2 and mensaje[posicion] != '\0') {

		if (mensaje[posicion]  == '&') {
			copiarPath = true;
			posicion += 6;
		}

		if (copiarPath){
			if (mensaje[posicion] != ' '){
				*strPathArchivo += mensaje[posicion];
			}

		}

		posicion++;
	}

}

string getPathArchivoRecibido(char mensaje[]){

	int posicion = 0;
	bool copiarPath = false;
	string strPathArchivo;

	while (posicion < MAX_LARGO_MENSAJE - 2 and mensaje[posicion] != '\0') {

		if (mensaje[posicion]  == '&') {
			copiarPath = true;
			posicion += 6;
		}

		if (copiarPath){
			if (mensaje[posicion] != ' '){
				strPathArchivo += mensaje[posicion];
			}

		}

		posicion++;
	}

return strPathArchivo;
}

bool verificarArchivo(char mensaje[]){

	int posicion = 0;
	char letra = mensaje[0];

	while (letra !='\n' and posicion < MAX_LARGO_MENSAJE - 2) {
		letra = mensaje[posicion];

		if (letra == '&') {
			return true;
		}
		posicion ++;
	}
	return false;
}

void leer_mensaje_escrito (char mensaje[]) {

	int posicion = 0;
	char letra;

	letra = getchar();
	while (letra !='\n' and posicion < MAX_LARGO_MENSAJE - 2) {

		mensaje[posicion] = letra;
		posicion++;
		letra = getchar();
	}

	mensaje[posicion] = '\0';
}

void inicioSignals () {

	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &manejadorSenhales;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGKILL, &sa, NULL);
	signal(SIGALRM, SIG_IGN);
}

void enviarArchivo(FILE *file, int fd, struct sockaddr_in server){
	int n;
	char buffer[MAX_LARGO_MENSAJE];
	unsigned int sin_size = sizeof(struct sockaddr_in);

	while (fgets(buffer, MAX_LARGO_MENSAJE, file) != NULL ){

	//	printf("[ENVIANDO] Archivo: %s", buffer);

		if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1){
			cout << "\33[46m\33[31m[ERROR]:" << " ERROR: enviando archivo.\33[00m\n";
			exit(1);
		}

		bzero(buffer, MAX_LARGO_MENSAJE);
	}

	//Le digo al servidor que termino el envio del archivo
  strcpy(buffer, "finArchivo");
  sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size);
 // printf("[ENVIANDO] Archivo enviado correctamente: %s\n", buffer);
  bzero(buffer, MAX_LARGO_MENSAJE);
}


void recibirArchivo(int fd, struct sockaddr_in server, char mensaje[]){

	char* filename;
	char buffer[MAX_LARGO_MENSAJE];
	sockaddr_in addr_size;
	string strPathArchivo;
	FILE *redes_file;
	char pathArchivo[MAX_LARGO_MENSAJE];

	strPathArchivo = getPathArchivoRecibido(mensaje);

	filename = (char *)strPathArchivo.c_str();
	redes_file = fopen(filename, "w");


	while (true){

	  unsigned int sin_size = sizeof(struct sockaddr_in);

		if(recvfrom(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr *)&server, &sin_size) == -1){
		  	cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible hacer recvfrom() para recepcion.\33[00m\n";
			exit(1);
		}

		if (strcmp(buffer, "finArchivo") == 0) {
	    //	printf("[RECIVIENDO] Fin: %s", buffer);
	    	fclose(redes_file);
	    	break;
	    }


		fprintf(redes_file, "%s", buffer);
		//printf("[RECIVIENDO] Archivo: %s", buffer);
		bzero(buffer, MAX_LARGO_MENSAJE);
	}

}


void autenticarUsuario (char usuario[MAX_NOMBRE], char clave[MAX_NOMBRE], char * argv[4]) {

	char buf[MAX_LARGO_MENSAJE];
	char usuariocontrasena[MAX_LARGO_MENSAJE];
	char contrasenaMD5[MAX_NOMBRE];
	char echo[MAX_NOMBRE];

	struct sockaddr_in server;
	struct hostent *he;

	int numbytes;
	int fd;
	int error;

	echo[0] = 0;
	strcat(echo, "echo -n ");
	strcat(echo, "'");
	strcat(echo, clave);
	strcat(echo, "'");
	strcat(echo, " | md5sum > md5.txt");

	system(echo);
	md5(contrasenaMD5);
 	char *token = strtok(contrasenaMD5, "-");

 	usuariocontrasena[0] = 0;
	strcat(usuariocontrasena, usuario);
	strcat(usuariocontrasena, "-");
	strcat(usuariocontrasena, contrasenaMD5);
	strcat(usuariocontrasena, "\r\n");

	if ((he = gethostbyname(argv[2])) == NULL) {

		cout << "\33[46m\33[31m[ERROR]:" << " gethostbyname()\33[00m\n";
		exit(-1);
	}

	fd = socket(AF_INET, SOCK_STREAM, 0); //socket tcp
	if (fd == -1 ) {

		cout << "\33[46m\33[31m[ERROR]:" << " socket()\33[00m\n";
		exit(-1);
	}

	server.sin_family = AF_INET;
   	server.sin_port = htons(atoi(argv[3]));
	server.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(server.sin_zero),8);

	error = connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr));

	if (error == -1) {

		cout << "\33[46m\33[31m[ERROR]:" << " connect()\33[00m\n";
		exit(-1);
	}

	numbytes = recv(fd, buf, MAX_LARGO_MENSAJE,0);

	if (numbytes == -1) {

		cout << "\33[46m\33[31m[ERROR]:" << " recv()\33[00m\n";
		exit(-1);
	}

	buf[numbytes - 2] = '\0';

	send(fd, usuariocontrasena, strlen(usuariocontrasena), 0);

	numbytes = recv(fd, buf, MAX_LARGO_MENSAJE,0);

	if (numbytes == -1) {

		cout << "\33[46m\33[31m[ERROR]: recv()\33[00m\n";
		exit(-1);
	}

	buf[numbytes - 2] = '\0';

	if (strcmp (buf,"NO") == 0) {

		cout << "\33[46m\33[31m[ERROR]: Imposible autenticar, usuario no valido.\33[00m\n";
		exit(-1);

	} else if (strcmp (buf,"SI") == 0) {

		numbytes = recv(fd, buf, MAX_LARGO_MENSAJE,0);

		if (numbytes == -1) {

			cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Al conectar.\33[00m\n";
			exit(-1);
		}

		buf[numbytes - 2] = '\0';
		cout << "Bienvenid@ " << buf << endl;

	} else {

		cout << "\33[46m\33[31m[ERROR]: Error en protocolo de autenticacion.\33[00m\n";
		exit(-1);
	}

	close(fd);

}

void recepcionMensajeria (int puerto) {

	int fd;
	int numbytes;

	char buffer[MAX_LARGO_MENSAJE];

	struct sockaddr_in server;
	struct sockaddr_in client;

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ) {

		cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible crear socket udp.\33[00m\n";
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(puerto);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server.sin_zero),8);

	if (bind(fd,(struct sockaddr*)&server,sizeof(struct sockaddr)) == -1) {

		cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible hacer bind() para recepcion.\33[00m\n";
		exit(1);
	}

	unsigned int sin_size = sizeof(struct sockaddr_in);

	while (true) {

		if ((numbytes = recvfrom(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr *)&client, &sin_size)) == -1) {

			cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible hacer recvfrom() para recepcion.\33[00m\n";
			exit(1);
		}

		buffer[numbytes -2] = '\0';

		if (verificarArchivo(buffer)){
			printf("%s %s %s\n", getTiempo(), inet_ntoa(client.sin_addr), buffer);
			recibirArchivo(fd, client, buffer);
		}else{
			printf("%s %s %s\n", getTiempo(), inet_ntoa(client.sin_addr), buffer);
		}

	}
	close(fd);
}

void envioMensajeria (int puerto, char usuario[MAX_NOMBRE]) {

	int fd;
	int broadcast;

	char mensaje[MAX_LARGO_MENSAJE];
	char buffer[MAX_LARGO_MENSAJE];
	char pathArchivo[MAX_LARGO_MENSAJE];
	string strPathArchivo;
	FILE * redes_file;

	struct hostent *he;

	struct sockaddr_in server, client;

	while (true) {

		if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ) { //socket UDP
			cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible abrir socket udp para envio.\33[00m\n";
			exit(1);
			}

		unsigned int sin_size = sizeof(struct sockaddr_in);

		char ip[25];
		cin >> ip;

		if (strcmp(ip, "*") == 0){  // broadcast

			broadcast = 1;

			server.sin_family = AF_INET;
			server.sin_port = htons(puerto);
			server.sin_addr.s_addr = INADDR_BROADCAST;
			bzero(&(server.sin_zero),8);

			if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {

				cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible hacer setsockopt() para envi   o.\33[00m\n";
				exit(1);
			}

		}else{ //no broadcast

			if ((he = gethostbyname(ip)) == NULL) {

				cout << "\33[46m\33[31m[ERROR]:" << " gethostbyname()\33[00m\n";
				exit(-1);
			}

			server.sin_family = AF_INET;
			server.sin_port = htons(puerto);
			server.sin_addr = *((struct in_addr *)he->h_addr);
			bzero(&(server.sin_zero),8);

		}
			leer_mensaje_escrito(mensaje);


			if (verificarArchivo(mensaje) == true){

				buffer[0] = '\0';

			    buffer[0] = '\0';
			    strcpy(buffer,usuario);
			    strcat(buffer," [Envio de archivo] ");
			    strcat(buffer,mensaje);
			    strcat(buffer,"\0");

				if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1) {

					cout << "\33[46m\33[31m[ERROR]:" << " ERROR: sendto().\33[00m\n";
					exit(1);
				}


	           	getPathArchivo(&strPathArchivo,mensaje);
				strcpy(pathArchivo, strPathArchivo.c_str());
				redes_file = fopen(pathArchivo,"rb");

				if(redes_file == NULL){
					cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible leer el archivo.\33[00m\n";
					exit(-1);
				}

				enviarArchivo(redes_file, fd, server);
				fclose(redes_file);

			}else{

				buffer[0] = '\0';
			    strcpy(buffer,usuario);
			    strcat(buffer," Dice: ");
			    strcat(buffer,mensaje);
			    strcat(buffer,"\0");

				if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1) {

					cout << "\33[46m\33[31m[ERROR]:" << " ERROR: sendto().\33[00m\n";
					exit(1);
				}
			}
	}
	close(fd);
}

int main (int argc, char * argv[]) {

//      - argv[0] es el string "mensajeria", puede cambiar si se llama con otro path.
//      - argv[1] es el puerto de escucha de mensajeria.
//      - argv[2] es el ip del servidor de autenticacion.
//      - argv[3] es el puerto del servidor de autenticacion.

	if (argc < 4) {

		cout << "\33[46m\33[31m[ERROR]:" << " Faltan argumentos: port, ipAuth, portAuth.\33[00m\n";
		exit (1);
	}

	inicioSignals();

    // ************* FILE ************************

    FILE * redes_file;

	//***********   Autenticacion  *****************

	char usuario[MAX_NOMBRE];
	char clave[MAX_NOMBRE];

	cout << "Usuario: ";
	cin >> usuario;
	cout << "Clave: ";
	cin >> clave;

	autenticarUsuario(usuario, clave, argv);

//*********** ***************** ***************** ***************** *****************

	char buffer[MAX_LARGO_MENSAJE];

	int fd;

	int pid = fork();

	int puerto = atoi(argv[1]);

	/**************** IMPOSIBLE BIFURCAR ****************/
	if (pid < 0) {

		cout << "\33[46m\33[31m[ERROR]:" << " Imposible Bifurcar.\33[00m\n";
		exit (1);
	}

	/**************** RECEPCION DE MENSAJES ****************/
	if (pid == 0) {
		recepcionMensajeria(puerto);
	}

	/********************* ENVIO DE MENSAJES *********************/
	if (pid > 0) {
		envioMensajeria(puerto, usuario);
	}
}
