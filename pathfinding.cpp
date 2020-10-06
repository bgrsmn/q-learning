//////////////////////////////////
//		Created By				//
//      						//
//   - Okan Pehlivan			//
//   - Bu�ra �i�man				//
//								//
//////////////////////////////////

#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <string.h>
#include "utils.h"
#include <time.h>

#define MAX_EPISODE 10000
#define alpha 0.8

using namespace std;


double R[100][100];
double Q[100][100]; // 0 de�erli g�ncellenecek
int possible_action[20];
int possible_action_num;


class Example : public olc::PixelGameEngine
{
public:
	int nMapWidth; //Matrisin s�tun say�s�
	int nMapHeight; //Matrisin sat�r say�s�
	int targetRoom; //Hedef nokta.
	int roomValue = 0; //Oda numaras�.
	bool qLearningDone = false;
	int nCellSize = 32; //G�r�n�m i�in gereken boyut bilgisi
	int nBorderWidth = 4; //G�r�n�m i�in gereken boyut bilgisi

	int nStartX; //Ba�lang�� noktas�n�n X kordinat�
	int nStartY; //Ba�lang�� noktas�n�n Y kordinat�
	int nEndX;   //Hedef noktas�n�n X kordinat�
	int nEndY;   //Hedef noktas�n�n Y kordinat�

	bool* bObstacleMap; //Default olarak false tan�mlanm�� engellerin bulundu�u dizi
	int* nFlowFieldZ;
	double** reduceMatrix; //�d�l tablosunu elde edece�imiz , �d�l matrisi

	Example()
	{
		sAppName = "Example";
	}

public:
	bool OnUserCreate() override
	{
		bObstacleMap = new bool[nMapWidth * nMapHeight]{ false }; //Engellerin 1 boyutlu dizi olarak tan�mlanmas� ve -1 de�erini almas� sa�land�.
		nFlowFieldZ = new int[nMapWidth * nMapHeight]{ 0 }; //Oda numaralar� en ba�ta 0 olarak atand�
		return true;
	}

	//�d�l matrisinin dinamik olarak olu�turulmas�.
	bool setReduceMatrix()
	{
		int size = nMapWidth * nMapHeight; //�d�l matrisinin boyutu

		reduceMatrix = new double* [size];

		for (int x = 0; x < size; x++)
		{
			reduceMatrix[x] = new double[size];
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				reduceMatrix[x][y] = -1.0; //Default olarak her bir de�er -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				R[x][y] = -1.0; //Default olarak her bir de�er -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				Q[x][y] = 0; //Default olarak her bir de�er 0 olarak belirlendi.
			}
		}

		return true;
	}

	//Terminal �zerinden bilgilendirmelerin yap�lmas� sa�land�.
	void Menu(string note) {

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << " Sol tik ile -> ENGEL , Sag tik ile -> BASLANGIC , Orta Tik ile -> HEDEF" << endl;

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << note << endl;

		cout << "----------------------------------------------------------------------------------" << endl;
	}


	//�d�l matrisinin hesaplanmas�.
	bool calculateReduceMatrix()
	{
		int size = nMapWidth * nMapHeight; //Boyut.

		//Olu�turulan map'in i�erisinde kalmas�n� ve engeller veya hedef konumuna gelip gelmemesinin kontrolleri yap�lmaktad�r.
		//Hedef noktas� = 100 puan , Engeller = -1 , Ba�ka bir odaya ge�ebilme durumu = 0
		for (int x = 0; x < nMapHeight; x++)
		{
			for (int y = 0; y < nMapWidth; y++)
			{
				int self = x * nMapWidth + y; //Matristeki her h�crenin kendi numaras� elde edilir. (Oda numaras�)

				//Sat�r numaras� 0 � g�sterdi�inde �st k�s�mda h�creler olmad��� i�in x != 0 �art� getirilip �st konunma bak�lmamas� sa�lanm��t�r.
				if (x != 0)
				{
					//H�crenin yukar�s�na bak�lmas� i�in yap�lan hesaplama
					int upper_neighbour = (x - 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , bo� oda ise = 0
					if (!bObstacleMap[upper_neighbour]) {
						if ((x - 1) == nEndX && y == nEndY) {
							reduceMatrix[self][upper_neighbour] = 100;
						}
						else {
							reduceMatrix[self][upper_neighbour] = 0;
						}
					}
				}

				//S�tun numaras� 0 � g�sterdi�inde sol k�s�mdaki h�creler olmad��� i�in y != 0 �art� getirilip sol taraf�n konumuna bak�lmamas� sa�lanm��t�r.
				if (y != 0)
				{
					//H�crenin sol taraf�na bak�lmas� i�in yap�lan hesaplama
					int left_neighbour = x * nMapWidth + y - 1;

					//Engel yoksa , hedef nokta ise = 100 , bo� oda ise = 0
					if (!bObstacleMap[left_neighbour]) {
						if (x == nEndX && (y - 1) == nEndY) {
							reduceMatrix[self][left_neighbour] = 100;
						}
						else {
							reduceMatrix[self][left_neighbour] = 0;
						}
					}
				}

				//Sa� tarafta kalan en son s�tun numaras� olmad��� s�rece yap�lan kontrold�r. Burada da sa� k�s�mda h�creler olmad��� i�in bu ko�ul konulmu�tur.
				if (y != nMapWidth - 1)
				{
					//H�crenin sa� taraf�na bak�lmas� i�in yap�lan hesaplama
					int right_neighbour = x * nMapWidth + y + 1;

					//Engel yoksa , hedef nokta ise = 100 , bo� oda ise = 0
					if (!bObstacleMap[right_neighbour])
					{
						if (x == nEndX && (y + 1) == nEndY)
						{
							reduceMatrix[self][right_neighbour] = 100;
						}
						else {
							reduceMatrix[self][right_neighbour] = 0;
						}
					}
				}

				//Alt tarafta kalan en son sat�r numaras� olmad��� s�rece yap�lan kontrold�r. Burada da a�a�� k�s�mda h�creler olmad��� i�in bu ko�ul konulmu�tur.
				if (x != nMapHeight - 1)
				{
					//H�crenin alt taraf�na bak�lmas� i�in yap�lan hesaplama
					int lower_neighbour = (x + 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , bo� oda ise = 0
					if (!bObstacleMap[lower_neighbour])
					{
						if ((x + 1) == nEndX && y == nEndY) {
							reduceMatrix[self][lower_neighbour] = 100;
						}
						else {
							reduceMatrix[self][lower_neighbour] = 0;
						}
					}
				}

			}
		}

		//Terminalde �d�l matrisinin g�r�nt�lenmesi
		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				cout << reduceMatrix[x][y] << " ";
			}
			cout << endl;
		}

		return true;
	}


	//Aray�zden al�nan �d�l matrisi ile global dizi olarak tan�mlanan R matrisinin e�itlenmesi
	bool EqualRmatrix() {

		int sizeReduce = nMapHeight * nMapWidth; //�d�l matrisinin boyutu

		for (int i = 0; i < sizeReduce; i++)
		{
			for (int j = 0; j < sizeReduce; j++)
			{
				R[i][j] = reduceMatrix[i][j];
			}
		}

		return true;
	}


	//M�mk�n olan , hareket edebilece�i yerlere gidiyor. possible_action[] dizisinde gidebilece�i oda numaralar�n� tutuyo
	void get_possible_action(double R[100][100], int state, int possible_action[20], int ACTION_NUM) {
		possible_action_num = 0;
		for (int i = 0; i < ACTION_NUM; i++) {
			if (R[state][i] >= 0) {
				possible_action[possible_action_num] = i;
				possible_action_num++;
			}
		}
	}

	//Q tablosunda bulundu�umuz sat�rda yani mevcut konumda maksimum de�eri bulup d�nd�r�yor.
	double get_max_q(double Q[100][100], int state, int ACTION_NUM) {
		double temp_max = 0;
		for (int i = 0; i < ACTION_NUM; ++i) {
			if ((R[state][i] >= 0) && (Q[state][i] > temp_max)) {
				temp_max = Q[state][i];
			}
		}
		return temp_max;
	}

	int episode_iterator(int init_state, double Q[100][100], double R[100][100], int STATE_NUM, int DES_STATE) {

		double Q_before;
		double Q_after;
		// yeni hareket
		int next_action;
		double max_q;

		int step = 0;
		while (1) {
			cout << "-- step " << step << ": initial state: " << init_state << endl;

			// memset possible_action dizi olarak tan�ml�yor.
			memset(possible_action, 0, 10 * sizeof(int)); //10 tane int de�erli dizi tan�mlama her de�er 0
			get_possible_action(R, init_state, possible_action, STATE_NUM);

			next_action = possible_action[rand() % possible_action_num];
			cout << "-- step " << step << ": next action: " << next_action << endl;

			max_q = get_max_q(Q, next_action, STATE_NUM);

			Q_before = Q[init_state][next_action];
			// Form�l g�ncelleme Q(s,a)=R(s,a)+alpha * max{Q(s', a')}
			Q[init_state][next_action] = R[init_state][next_action] + alpha * max_q; //Q learning formul�
			Q_after = Q[init_state][next_action];

			if (next_action == DES_STATE) {
				init_state = rand() % STATE_NUM;
				break;
			}
			else {
				// Hedef konumu bulamad�ysam �uan ki konumum yeni denedi�im konumdur.
				init_state = next_action;
			}
			step++;
		}

		return init_state;
	}

	int inference_best_action(int now_state, double Q[100][100], int ACTION_NUM) {
		// En son Q matrisinde 
		double temp_max_q = 0;
		int best_action = 0;
		for (int i = 0; i < ACTION_NUM; ++i) {
			if (Q[now_state][i] > temp_max_q) {
				temp_max_q = Q[now_state][i];
				best_action = i;
			}
		}
		return best_action;
	}

	void run_training(int init_state, int MATRIX_ROW, int MATRIX_COLUMN, int targetRoom) {

		int initial_state = init_state; // Denemeye ba�layaca�� oda numaras� 
		int sizeReduce1 = MATRIX_ROW; //episode_iterator fonksiyonuna yollanmak i�in tan�mlanm��t�r.

		// Rastgele ba�lad��� durum.
		srand((unsigned)time(NULL));
		cout << "[INFO] start training..." << endl;
		for (int i = 0; i < MAX_EPISODE; ++i) {
			cout << "[INFO] Episode: " << i << endl; //ka� b�l�mde buldu�unu g�rmek i�in her d�ng�de yazd�r�yor.
			initial_state = episode_iterator(initial_state, Q, R, sizeReduce1, targetRoom);
			cout << "-- updated Q matrix: " << endl;
			print_matrix(Q, MATRIX_ROW, MATRIX_COLUMN);
		}
	}


	//Q learning algoritmas�n�n hesaplanmas�n� sa�layan fonksiyon
	bool Qlearning()
	{
		int sizeReduce = nMapHeight * nMapWidth; //�d�l matrisi boyutu

		cout << "Q matris:" << endl;
		print_matrix(Q, sizeReduce, sizeReduce);
		cout << "R matris:" << endl;
		print_matrix(R, sizeReduce, sizeReduce);

		run_training(1, sizeReduce, sizeReduce, targetRoom);

		int startPosition = nStartY * nMapWidth + nStartX; // Ba�lang�� konumunun numaras�

		NewCalculate(startPosition); //Aray�zde de�i�tirilen ba�lang�� noktas�n�n parametre olarak yollanmas�.
		qLearningDone = true;

		return true;
	}

	//En k�sa yolun g�sterilmesi , aray�z �zerinde de en k�sa yol g�sterilirken oda numaralar� -1 yap�lm��t�r.
	void NewCalculate(int position)
	{
		int sizeReduce = nMapHeight * nMapWidth;

		for (int i = 0; i < sizeReduce; i++)
		{
			nFlowFieldZ[i] = i;
		}

		cout << "Robotun guzergahi " << endl;
		cout << position << "->";

		while (true) {
			int best_action = inference_best_action(position, Q, sizeReduce);
			cout << best_action << "->";

			nFlowFieldZ[position] = -1;


			if (best_action == targetRoom) {
				cout << "out" << endl;
				break;
			}
			else {
				position = best_action;
			}

		}

	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//Burada her h�crenin kendi numaras�n� elde etmek i�in p fonksiyonu yaz�ld�.
		auto p = [&](int x, int y) { return y * nMapWidth + x; };
		int size = nMapWidth * nMapHeight; //Boyut.

		//T�klanan kutucu�un X ve Y kordinatlar�.
		int nSelectedCellX = GetMouseX() / nCellSize;
		int nSelectedCellY = GetMouseY() / nCellSize;

		//Sol t�k ile t�klay�nca engel olu�turulmas�. Engel �st�ne bir kez daha t�klay�nca engel kalkar.
		if (GetMouse(0).bReleased)
		{
			bObstacleMap[p(nSelectedCellX, nSelectedCellY)] = !bObstacleMap[p(nSelectedCellX, nSelectedCellY)];
		}

		//Ba�lang�� durumunun X ve Y kordinatlar� al�nmas�.
		if (GetMouse(1).bReleased)
		{
			nStartX = nSelectedCellX;
			nStartY = nSelectedCellY;

			//Q learning algoritmas� hesapland�ktan sonra ba�lang�� konumunun yeni Q learning hesab� i�in NewCalculate() fonksiyonuna g�nderilmesi.
			if (qLearningDone)
			{
				int position = nStartY * nMapWidth + nStartX;
				NewCalculate(position);
			}

		}

		//Hedef durumunun X ve Y kordinatlar�n�n al�nmas�.
		if (GetMouse(2).bReleased)
		{
			nEndX = nSelectedCellY;
			nEndY = nSelectedCellX;

			targetRoom = nEndX * nMapWidth + nEndY; //Hedef noktan�n numaras�.

			qLearningDone = false;
			setReduceMatrix(); //Yeniden hesaplama yap�laca�� zaman matrislerin de�erlerinin ba�lang�� konumuna getirilmesi. 
		}

		//Engeller konulup , ba�lang�� ve hedef noktalar� belirlendikten sonra
		//A tu�una bas�ld���nda �d�l matrisinin hesaplanmas� yap�l�r.
		if (GetKey(olc::Key::A).bReleased)
		{
			cout << "--------------------ODUL MATRISI---------------------------" << endl;
			setReduceMatrix();
			calculateReduceMatrix(); //�d�l matrisinin hesaplanmas�.
			EqualRmatrix();

			cout << "------------------------------------------------" << endl;
			string note = "QLearning algoritmasini calistirmeak icin (Q) tusuna basiniz...";
			Menu(note);
		}

		//Q learning algoritmas�n�n hesaplanmas�
		if (GetKey(olc::Key::Q).bReleased)
		{
			Qlearning();
		}

		//Her odan�n numaras�n�n hesaplanmas� (1,2,3,4,5,....)
		while (roomValue < size)
		{
			for (int x = 0; x < nMapHeight; x++)
			{
				for (int y = 0; y < nMapWidth; y++)
				{
					nFlowFieldZ[p(y, x)] = roomValue++;
				}
			}
		}

		//Varsay�lan kutucuklar�n mavi renk olmas�.
		olc::Pixel colour = olc::BLUE;

		//Ba�lang�� , engel ve hedef noktalar�n�n renklerinin belirlenmesi.
		for (int x = 0; x < nMapWidth; x++)
		{
			for (int y = 0; y < nMapHeight; y++)
			{
				olc::Pixel colour = olc::BLUE;

				if (bObstacleMap[p(x, y)])
					colour = olc::GREY;

				if (x == nStartX && y == nStartY)
					colour = olc::GREEN;

				if (x == nEndY && y == nEndX)
					colour = olc::RED;

				FillRect(x * nCellSize, y * nCellSize, nCellSize - nBorderWidth, nCellSize - nBorderWidth, colour);
				DrawString(x * nCellSize, y * nCellSize, std::to_string(nFlowFieldZ[p(x, y)]), olc::WHITE); //Oda numaralar� g�sterilmesi
			}
		}

		return true;
	}
};


int main()
{
	Example demo;

	//Kullan�c�dan sat�r , s�tun , hedef noktas�n�n al�nmas� , Hedef noktas� burada girildikten sonra aray�zde de de�i�tirilince bir s�k�nt� olmuyor.
	while (true) {
		cout << "Satir, Sutun ve Hedef Noktasini 0(Sifir)dan buyuk giriniz!" << endl;
		cout << endl;
		cout << "Satir Sayisini Giriniz : ";
		cin >> demo.nMapHeight;
		cout << "Sutun Sayisini Giriniz : ";
		cin >> demo.nMapWidth;

		if (demo.nMapHeight < 0 || demo.nMapWidth < 0)
		{
			cout << "Giris HATALI!!!!" << endl;
			continue;
		}

		int size = demo.nMapHeight * demo.nMapWidth;

		cout << "Hedef Noktasini Giriniz (0 ile " << size << " arasinda) giriniz: ";
		cin >> demo.targetRoom;

		if (demo.targetRoom >= size) cout << "Giris HATALI!!!" << endl;
		else break;
	}

	//Hedef noktas�n�n X ve Y kordinatlar� 
	demo.nEndX = demo.targetRoom / demo.nMapWidth;
	demo.nEndY = demo.targetRoom % demo.nMapWidth;

	string note = " Baslangic, hedef ve engelleri belirledikten sonra \n Odul matrisini olusturmak icin (A) tusuna basiniz.";
	demo.Menu(note);

	cout << endl;

	demo.setReduceMatrix(); //�d�l matrisini boyutland�rma.

	if (demo.Construct(512, 480, 2, 2))
	{
		demo.Start();
	}

	return 0;
}