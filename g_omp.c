/*Tema 1 APD - Game of Life
  Student Mustata Alexandru-Ionut
  Grupa 331CB
*/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/*Functia aloca spatiul de memorie necesar pentru
o matrice. Daca nu se poate aloca spatiul se intoarce
NULL.*/
char** newMatrix(int lines, int columns){
	char** matrix = (char**)malloc(lines*sizeof(char*));
	if(matrix == NULL){
		return NULL;
	}
	
	int i=0, j=0;
	for(i=0; i<lines; i++){
		matrix[i] = (char*)malloc(columns*sizeof(char));
		if(matrix[i] == NULL){
			for(j=0; j<i; j++){
				free(matrix[j]);
			}
			return NULL;
		}
	}
	return matrix;
}

/*Functia elibereaza spatiul ocupat de o matrice in memorie.*/
void deleteMatrix(char*** matrix, int lines, int columns){
	char** aux = *matrix;
	int i=0;
	for(i=0; i<lines; i++){
		free(aux[i]);
	}	
	free(aux);
	*matrix = NULL;
}

/*Functia copiaza elementele din matricea matrix2 in matricea
matrix1. Paralelizarea functiei este realizata prin copierea
paralela a liniilor.*/
void copyToFrom(char** matrix1, char** matrix2, int lines, int columns){
	int i=0, j=0;
	#pragma omp parallel for private(i,j)
	for(i=0; i<lines; i++){
		for(j=0; j<columns; j++){
			matrix1[i][j] = matrix2[i][j];
		}
	}
}

/*Functia realizeaza refacerea marginilor matricii pentru
jocul Game of Life dupa ce s-a calculat continutul efectiv.
Paralelizarea se aplica prin copierea in paralel a elementelor
de pe prima si ultima linie si pentru elementele de pe prima
si ultima coloana.*/
void updateEdges(char** matrix, int lines, int columns){
	int i=0;

	//Commpletare celule suplimentare.
	
	//Completare linie superioara si inferioara.
	#pragma omp parallel for private(i)
	for(i=1; i<columns+1; i++){
		matrix[0][i] = matrix[lines][i];
		matrix[lines+1][i] = matrix[1][i];
	}

	//Completare prima si ultima coloana.
	#pragma omp parallel for private(i)
	for(i=1; i<lines+1; i++){
		matrix[i][0] = matrix[i][columns];
		matrix[i][columns+1] = matrix[i][1];
	}

	//Completare colturi.
	matrix[0][0] = matrix[lines][columns];
	matrix[0][columns+1] = matrix[lines][1];
	matrix[lines+1][0] = matrix[1][columns];
	matrix[lines+1][columns+1] = matrix[1][1];
}

/*Functia realizeaza citirea unei matrici ce respecta
formatul din enunt dintr-un fisier de intrare.*/
char** readMatrixFromFile(FILE* in, int lines, int columns){
	char** matrix = newMatrix(lines+2, columns+2);

	if(matrix == NULL){
		return NULL;
	}

	int i=0, j=0;

	for(i=1; i<lines+1; i++){
		char c;
		for(j=1; j<columns+1; j++){
			fscanf(in, "%c%c", &matrix[i][j], &c);
		}
		fscanf(in, "%c", &c);
	}

	updateEdges(matrix, lines, columns);

	return matrix;
}

/*Functia scrie intr-un fisier de iesire matricea,
respectand formatul din enunt.*/
void writeMatrixToFile(FILE* out, char** matrix, int lines, int columns){
	int i=0, j=0;
	for(i=1; i<lines+1; i++){
		for(j=1; j<columns+1; j++){
			fprintf(out, "%c ", matrix[i][j]);
		}
		if(i != lines) fprintf(out, "\n");
	}
}

/*Functia calculeaza numarul de vecini vii ai unei celule.*/
int countAliveNeighbors(char** matrix, int x, int y){
	int n=0;
	//X -> ALIVE
	//. -> DEAD
	if(matrix[x-1][y-1] == 'X') n++;
	if(matrix[x-1][y] == 'X') n++;
	if(matrix[x-1][y+1] == 'X') n++;
	if(matrix[x][y-1] == 'X') n++;
	if(matrix[x][y+1] == 'X') n++;
	if(matrix[x+1][y-1] == 'X') n++;
	if(matrix[x+1][y] == 'X') n++;
	if(matrix[x+1][y+1] == 'X') n++;
	return n;
}

/*Functia primeste coordonatele unei celule si starea aceteia
si returneaza starea celulei la urmatoarea iteratie.*/
char getNextState(char** matrix, int x, int y){
	int n = countAliveNeighbors(matrix, x, y);
	char c = '#';
	if(n<2){
		c = '.';
	}else if(matrix[x][y]=='X' && (n==2 || n==3)){
		c = 'X';
	}else if(n==3 && matrix[x][y]=='.'){
		c = 'X';
	}else if(n>3){
		c = '.';
	}else{
		c = matrix[x][y];
	}
	return c;
}

/*Functia primeste doua zone de memorie (matrici) unde va desfasura
calculele pentru a determina starea jocului dupa steps iteratii.
matrix1 reprezinta configuratia initiala a jocului. Dupa executia
functiei, matrix2 va reprezenta starea care se dorea calculata.*/
void applyEvolution(char** matrix1, char** matrix2, int lines, int columns, int steps){
	int i=0, j=0, step=0;
	for(step=0; step<steps; step++){

		/*Paralelizarea calculului stari viitoare se executa in trei etape:
			-> Se calculeaza liniile cu indici 3k+1;
			-> Se calculeaza liniile cu indici 3k+2;
			-> Se calculeaza liniile cu indici 3k.
		  Am folosit aceasta metoda de a paraleliza algoritmul pentru a evita
		  cache false sharing (Nu trebuie ca la un moment de timp, o linie sa
		  fie analizata de doua thread-uri, deci se formeaza triplete de forma
		  <linie_vecini_sus, linie_de_calculat, linie_vecini_jos>).
		*/

		#pragma omp parallel for private(i,j)
		for(i=1; i<lines+1; i+=3){
			for(j=1; j<columns+1; j++){
				matrix2[i][j] = getNextState(matrix1, i, j);
			}
		}

		#pragma omp parallel for private(i,j)
		for(i=2; i<lines+1; i+=3){
			for(j=1; j<columns+1; j++){
				matrix2[i][j] = getNextState(matrix1, i, j);
			}
		}

		#pragma omp parallel for private(i,j)
		for(i=3; i<lines+1; i+=3){
			for(j=1; j<columns+1; j++){
				matrix2[i][j] = getNextState(matrix1, i, j);
			}
		}

		/*Se face update marginilor si se copiaza noua stare
		peste cea veche.*/
		updateEdges(matrix2, lines, columns);
		copyToFrom(matrix1, matrix2, lines+2, columns+2);
	}
}

int main(int argc, char** argv){
	if(argc != 4){
		/*Numar incorect de argumente.*/
		return 0;
	}

	int steps = atoi(argv[2]);

	/*Deschiderea fisierelor de input si output.*/
	FILE* in = fopen(argv[1],"r");
	FILE* out = fopen(argv[3],"w");

	int lines = 0, columns = 0;

	char c;
	fscanf(in,"%d%d%c",&lines,&columns,&c);

	/*Citirea starii initiale.*/
	char** matrix1 = readMatrixFromFile(in, lines, columns);
	if(matrix1 == NULL){
		fclose(in);
		fclose(out);
		return 0;
	}

	/*Alocarea de spatiu pentru matricea secundara.*/
	char** matrix2 = newMatrix(lines+2, columns+2);
	if(matrix2 == NULL){
		fclose(in);
		fclose(out);
		deleteMatrix(&matrix1, lines+2, columns+2);
		return 0;
	}
	
	/*Rularea functiei ce determina starea dupa steps iteratii.*/
	applyEvolution(matrix1, matrix2, lines, columns, steps);

	/*Scrierea rezultelor in fisierul de output.*/
	writeMatrixToFile(out, matrix2, lines, columns);

	/*Inchiderea fisierelor de input si output
	si eliberarea memoriei folosite.*/
	fclose(in);
	fclose(out);
	deleteMatrix(&matrix1, lines+2, columns+2);
	deleteMatrix(&matrix2, lines+2, columns+2);
	return 0;
}