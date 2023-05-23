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

string encryptMD5 (string pass) {
	//Generamos un archivo txt con el hash md5 pasado a consola
	string echo = "echo -n '" + pass + "' | md5sum > md5.txt" ;
	const char *c = echo.c_str();
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

	//se obtiene desde la posicion 3 ya que al leer agrega md5 al inicio y se utilizan los 32 caracteres del hash"

	return md5Pass.substr(0, 32);  //Preguntar porque no acepta tomar solo 32 bit "substr(3, 32)"
}

void recivir(int fd1, char *buffer)
{
	
	int numbytes = recv(fd1, buffer, MAX_LARGO_MENSAJE, 0);
	cout << "buffer: ";
	cout<< buffer << endl;
	if (numbytes == -1){  
		cout << "\33[46m\33[31m[ERROR]:" << "Al Recibir Mensaje.\33[00m\n";
		exit(-1);
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
	string user;
	string password;
	string auth;

	cout << "Ingrese Usuario: ";
	cin >> user;
	cout << "Ingrese Contraseña: ";
	cin >> password;
	cout << password <<endl;	
	auth = encryptMD5(password);


	cout << "salida md5 " + auth << endl;

	struct sockaddr_in server; 	 /* para la información de la dirección del servidor */

	//struct sockaddr_in client;       /* para la información de la dirección del cliente */
	
	struct hostent *he;         
	/* estructura que recibirá información sobre el nodo remoto */

	int fd1;

	char buff[MAX_LARGO_MENSAJE];

	/* A continuación la llamada a socket() */
	fd1 = socket(AF_INET, SOCK_STREAM, 0);
	if (fd1 == -1){  
      		cout << "\33[46m\33[31m[ERROR]: " << "Al Bindear\33[00m\n";
      		exit(-1);
	}
	
	//Seteo de estructura Server.
	he = gethostbyname(argv[2]);
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
	
	recivir(fd1,buff);		
	
	//Preparacion y envio de buffer
	memset (buff,0,sizeof(buff)) ;
	cout<<auth<<endl;
	strcat(buff,user.c_str());
	strcat(buff,"-");
	strcat(buff,auth.c_str());
	strcat(buff,"\r\n");
	send(fd1, buff , auth.length()+user.length()+3, 0); 
	cout << "buffer2: ";
	cout<< buff<< endl;
	memset (buff,0,sizeof(buff));
	recivir(fd1,buff);
	if ((buff[0] == 'N') && (buff[1] == 'O')) {
		cout << "\33[46m\33[31m[ERROR]: Imposible autenticar, usuario no valido.\33[00m\n";
		exit(-1);
	}	
	
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
		while (true){
			sleep(1);
			// Incluir el codigo de la recepcion de mensajeria y archivos.
		}
	}
	
	
	if (pid > 0){
		printf("\33[34mTx\33[39m: Iniciada parte que transmite mensajes. Pid %d\n", getpid());
		while (true){
			sleep(1);
			// Leer de la entrada estandar e incluir
			// el codigo de la emision de mensajeria y archivos.
		}
	}
}
