// Redes de Computadoras - Curso 1er Semestre 2023
// Tecnologo en Informatica FIng - CETP
//
// Entrega 2  - Programacion con Sockets
// Sistema basico de Mensajeria 

// Integrantes:
//	Damaso Tor - 4508724-7
//  Ezequiel Medina - 5527291-7
//  Franco Erramouspé - 5060103-4

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <arpa/inet.h> // para inet_Addr, etc
#include <netdb.h> // estrucrutas
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <sys/signal.h>
#include <ctype.h> 
#include <iostream>

#define MAX_LARGO_MENSAJE 255

using namespace std;

void manejadorSenhales (int signal){
// Manejador de las senhales.
// Aca se debe implementar la accion a realizar cuando se recibe la senhal
// Deberia haber un manejador de senhales para cada hijo si hacen cosas distintas
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

void resetString (char * & s){
// Resetea un string.
	s[0] = '\0';
}

char * agregarCero(char * cad,int num){
// Chequea si el num es < 10 y me devuelve un string con el '0'
// concatenado con num en dicho caso
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

char * getTiempo(){
// Obtiene la fecha y hora local y la almacena en un string
// con formato DD/MM/YYYY-hh:mm:ss
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

//Codifica la contrase;a en MD5
string encryptMD5 (string pass) {
	string echo = "echo -n '" + pass + "' | md5sum > md5.txt" ; 	//Generamos un archivo txt con el hash md5 pasado a consola
	const char *c = echo.c_str(); //La consola solo acepta arreglos de caracteres
	system(c);

	//Leemos el md5.txt para obtener la contraseña
	string md5Pass;

	ifstream myfile ("md5.txt");

	if (myfile.is_open()) {
        string line;
        while (getline(myfile, line)) {
            md5Pass += line;
        }
        myfile.close();
    } else {
        cout << "No se pudo abrir el archivo";
		exit(-1);
    }

	return md5Pass.substr(0, 32);  //Extrae solo los 32 caracteres del hash
}

//Funcion para recibir mensajes de paquetes y ponerlos en el buffer
void recibirMensaje(int fd1,char buff[]) {	
	strcpy(buff,"");
	int numbytes = recv(fd1, buff, MAX_LARGO_MENSAJE, 0);
	if (numbytes == -1){  
		cout << "\33[46m\33[31m[ERROR]:" << "Al Recibir Mensaje.\33[00m\n";
		exit(-1);
	}else {
		buff[numbytes-2] = '\0';
	}
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

void recepcionMensajeria (int puerto) {

	int fd;
	int numbytes;

	char buffer[MAX_LARGO_MENSAJE];
	string pathFile;

	struct sockaddr_in server;
	struct sockaddr_in client;

	FILE * redes_file;


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


		if (strcasecmp(buffer,"inicioArchivo")==0){ //Si es un archivo
			strcpy(buffer, "\0");
			redes_file = fopen("./descargado.png", "w");
			while (strcasecmp(buffer,"finArchivo")!=0){
				if ((numbytes = recvfrom(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr *)&client, &sin_size)) == -1) {
					cout << "\33[46m\33[31m[ERROR]:" << " ERROR: Imposible hacer recvfrom() para recepcion.\33[00m\n";
					exit(1);	
				}
				fprintf(redes_file, "%s", buffer);
			}						
			fclose(redes_file);

		}else {
			cout << getTiempo() << " " << inet_ntoa(client.sin_addr) << " " << buffer << endl;
		}


		


		/*if (verificarArchivo(buffer)){
			printf("%s %s %s\n", getTiempo(), inet_ntoa(client.sin_addr), buffer);
			recibirArchivo(fd, client, buffer);
		}else{
			;
		}*/
		}

	close(fd);

}

bool verificarArchivo(char buffer[]){
	char iter = buffer[0];
	bool retorno = false;
	int posicion = 0;
	while (iter != '\0' && !retorno && posicion <= MAX_LARGO_MENSAJE){
		iter = buffer[posicion];
		if (iter == '&') {
			retorno = true;
		}		
		else 
			posicion ++;
	}
	return retorno;
}


void envioMensajeria (int puerto, string usuario) {

	int fd;
	int broadcast;

	char mensaje[MAX_LARGO_MENSAJE];
	char buffer[MAX_LARGO_MENSAJE];
	char pathArchivo[MAX_LARGO_MENSAJE];
	string strPathArchivo;
	FILE * redes_file;
	int posicion = 0;

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


	if ((he = gethostbyname(ip)) == NULL) {
		cout << "\33[46m\33[31m[ERROR]:" << " gethostbyname()\33[00m\n";
		exit(-1);
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(puerto);
	server.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(server.sin_zero),8);

	leer_mensaje_escrito(mensaje);

	if (verificarArchivo(mensaje)){ //Si es un archivo
		char iter = mensaje[0];
		posicion=7; // espacio + &file + espacio
		while (iter != '\0' && posicion <= MAX_LARGO_MENSAJE){
			iter = mensaje[posicion];
			strPathArchivo +=iter;
			posicion++;
		}


		strcpy(buffer,usuario.c_str());
		strcat(buffer," [Envio de archivo] ");
		strcat(buffer,strPathArchivo.c_str());
		strcat(buffer,"\0");

		if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1) {

			cout << "\33[46m\33[31m[ERROR]:" << " ERROR: sendto().\33[00m\n";
			exit(1);
		}
		strcpy(buffer,"\0");



		strcpy(buffer, "inicioArchivo");
		sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size);
		strcpy(buffer,"\0");

		redes_file = fopen(strPathArchivo.c_str(),"rb");
		while (fgets(buffer, MAX_LARGO_MENSAJE, redes_file) != NULL ){
			
			if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1){
				cout << "\33[46m\33[31m[ERROR]:" << " ERROR: enviando archivo.\33[00m\n";
				exit(1);
			}
			
			bzero(buffer, MAX_LARGO_MENSAJE);
		}


		fclose(redes_file);
		strcpy(buffer, "finArchivo");
  		sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size);
  		bzero(buffer, MAX_LARGO_MENSAJE);

	}else {
		strcpy(buffer,usuario.c_str());
		strcat(buffer," Dice: ");
		strcat(buffer,mensaje);
		strcat(buffer,"\0"); 
	}
	

	if (sendto(fd, buffer, MAX_LARGO_MENSAJE, 0, (struct sockaddr*)&server, sin_size) == -1) {

		cout << "\33[46m\33[31m[ERROR]:" << " ERROR: sendto().\33[00m\n";
		exit(1);
	}

	
	}

}

int main(int argc, char * argv[]){
// En argc viene la cantidad de argumentos que se pasan,
// si se llama solo al programa, el nombre es el argumento 0
// En nuestro caso:
//      - argv[0] es el string "mensajeria", puede cambiar si se llama con otro path.
//      - argv[1] es el puerto de escucha de mensajeria.
//      - argv[2] es el ip del servidor de autenticacion.
//      - argv[3] es el puerto del servidor de autenticacion.

	if (argc < 4){
		cout << "\33[46m\33[31m[ERROR]:" << " Faltan argumentos: port, ipAuth, portAuth.\33[00m\n";
		exit (1);
	}

	// Estructuras para el manejo de Senhales
	// Deberia haber un manejador de senhales para cada hijo si hacen cosas distintas
	// *********************************
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &manejadorSenhales;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGKILL, &sa, NULL);
	signal(SIGALRM, SIG_IGN);
	// **********************************
	
	cout << "\33[34mRedes de Computadoras 2023\33[39m: Sistema de Mensajeria.\nEscuchando en el puerto " << argv[1] << ".\nProceso de pid: " << getpid() << ".\n";
	
	
	// Antes de iniciar el programa de mensajeria debe autenticarse
	// como especifica la letra
	
	//Definicion Estructuras de paquetes
	
	struct sockaddr_in server; 	 /* para la información de la dirección del servidor */
	//struct sockaddr_in client;       /* para la información de la dirección del cliente */	
	struct hostent *he;    /* estructura que recibirá información sobre el nodo remoto */

	
	//Variables para Autentificacion
	int fd1;
	char buff[MAX_LARGO_MENSAJE];
	string user;
	string password;
	string auth;
	int puerto = atoi(argv[1]);

	cout << "Ingrese Usuario: ";
	cin >> user;
	cout << "Ingrese Contraseña: ";
	cin >> password;
	auth = encryptMD5(password);

	
	/* A continuación la llamada a socket() */
	fd1 = socket(AF_INET, SOCK_STREAM, 0);
	if (fd1 == -1){  
      		cout << "\33[46m\33[31m[ERROR]: " << "Al Bindear\33[00m\n";
      		exit(-1);
	}
	
	//Seteo de estructura Server.
	he = gethostbyname(argv[2]);
	if (he == NULL){
		cout << "\33[46m\33[31m[ERROR]: " << "Al Resolver el DNS\33[00m\n";
		exit(-1);
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[3])); 
	server.sin_addr.s_addr = INADDR_ANY; 
	server.sin_addr = *((struct in_addr *)he->h_addr);  
	bzero(&(server.sin_zero),8);

	 /* llamada a connect() */
	int error = connect(fd1, (struct sockaddr *)&server,  sizeof(struct sockaddr)); 
	if(error == -1){ 
		cout << "\33[46m\33[31m[ERROR]: " << "Al Conectar\33[00m\n";
		exit(-1);
	}
	
	//Recepcion de respuesta
	recibirMensaje(fd1,buff);	


	//Chequeo del Saludo
	/*if (strcmp(buff,"Redes 2023 - Laboratorio - Autenticacion de Usuarios") != 0){
		cout << "\33[46m\33[31m[ERROR]: " << "Protocolo Incorrecto\33[00m\n" << endl;
		exit(-1);
	}*/
	
	//Preparacion y envio de buffer
	strcpy(buff,"");
	strcat(buff,user.c_str());
	strcat(buff,"-");
	strcat(buff,auth.c_str());
	strcat(buff,"\r\n");
	send(fd1, buff , auth.length() + user.length() + 3, 0); 

	recibirMensaje(fd1,buff);
	if(strcasecmp(buff,"NO")==0){
		cout << "\33[46m\33[31m[ERROR]: Imposible autenticar, usuario o contraseña Incorrecto.\33[00m\n";
		exit(-1);
	}else if (strcasecmp(buff,"SI") !=0){
		cout << "\33[46m\33[31m[ERROR]: Error Protocolo de autentificacion.\33[00m\n";
		exit(-1);
	}
	recibirMensaje(fd1,buff);

	cout << "Bienvenid@ " <<buff <<endl;

	close(fd1);
	

	// Una vez autenticado puede comenzar a recibir y empezar el mensajes y archivos.
	// Para esto se debe bifircar el programa.
	// Es indistinto si el padre transmite y el hijo recibe, o viceversa, lo que si
	// al ser distintos porcesos, van a tener distinto pid.
	// Familiarizarse con los comandos de UNIX ps, ps -as, kill, etc.
	
	int pid = fork();
	
	if (pid < 0){
		cout << "\33[46m\33[31m[ERROR]:" << " Imposible Bifurcar.\33[00m\n";
		exit (1);
	}
	
	if (pid == 0){
		printf("\33[34mRx\33[39m: Iniciada parte que recepciona mensajes. Pid %d\n", getpid());
		
		recepcionMensajeria(atoi(argv[1]));		
	}
	
	
	if (pid > 0){
		printf("\33[34mTx\33[39m: Iniciada parte que transmite mensajes. Pid %d\n", getpid());
		
		envioMensajeria(puerto, user);
	}
}
