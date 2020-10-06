//////////////////////////////////
//		Created By				//
//      						//
//   - Okan Pehlivan			//
//   - Buðra Þiþman				//
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
double Q[100][100]; // 0 deðerli güncellenecek
int possible_action[20];
int possible_action_num;


class Example : public olc::PixelGameEngine
{
public:
	int nMapWidth; //Matrisin sütun sayýsý
	int nMapHeight; //Matrisin satýr sayýsý
	int targetRoom; //Hedef nokta.
	int roomValue = 0; //Oda numarasý.
	bool qLearningDone = false;
	int nCellSize = 32; //Görünüm için gereken boyut bilgisi
	int nBorderWidth = 4; //Görünüm için gereken boyut bilgisi

	int nStartX; //Baþlangýç noktasýnýn X kordinatý
	int nStartY; //Baþlangýç noktasýnýn Y kordinatý
	int nEndX;   //Hedef noktasýnýn X kordinatý
	int nEndY;   //Hedef noktasýnýn Y kordinatý

	bool* bObstacleMap; //Default olarak false tanýmlanmýþ engellerin bulunduðu dizi
	int* nFlowFieldZ;
	double** reduceMatrix; //Ödül tablosunu elde edeceðimiz , ödül matrisi

	Example()
	{
		sAppName = "Example";
	}

public:
	bool OnUserCreate() override
	{
		bObstacleMap = new bool[nMapWidth * nMapHeight]{ false }; //Engellerin 1 boyutlu dizi olarak tanýmlanmasý ve -1 deðerini almasý saðlandý.
		nFlowFieldZ = new int[nMapWidth * nMapHeight]{ 0 }; //Oda numaralarý en baþta 0 olarak atandý
		return true;
	}

	//Ödül matrisinin dinamik olarak oluþturulmasý.
	bool setReduceMatrix()
	{
		int size = nMapWidth * nMapHeight; //Ödül matrisinin boyutu

		reduceMatrix = new double* [size];

		for (int x = 0; x < size; x++)
		{
			reduceMatrix[x] = new double[size];
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				reduceMatrix[x][y] = -1.0; //Default olarak her bir deðer -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				R[x][y] = -1.0; //Default olarak her bir deðer -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				Q[x][y] = 0; //Default olarak her bir deðer 0 olarak belirlendi.
			}
		}

		return true;
	}

	//Terminal üzerinden bilgilendirmelerin yapýlmasý saðlandý.
	void Menu(string note) {

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << " Sol tik ile -> ENGEL , Sag tik ile -> BASLANGIC , Orta Tik ile -> HEDEF" << endl;

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << note << endl;

		cout << "----------------------------------------------------------------------------------" << endl;
	}


	//Ödül matrisinin hesaplanmasý.
	bool calculateReduceMatrix()
	{
		int size = nMapWidth * nMapHeight; //Boyut.

		//Oluþturulan map'in içerisinde kalmasýný ve engeller veya hedef konumuna gelip gelmemesinin kontrolleri yapýlmaktadýr.
		//Hedef noktasý = 100 puan , Engeller = -1 , Baþka bir odaya geçebilme durumu = 0
		for (int x = 0; x < nMapHeight; x++)
		{
			for (int y = 0; y < nMapWidth; y++)
			{
				int self = x * nMapWidth + y; //Matristeki her hücrenin kendi numarasý elde edilir. (Oda numarasý)

				//Satýr numarasý 0 ý gösterdiðinde üst kýsýmda hücreler olmadýðý için x != 0 þartý getirilip üst konunma bakýlmamasý saðlanmýþtýr.
				if (x != 0)
				{
					//Hücrenin yukarýsýna bakýlmasý için yapýlan hesaplama
					int upper_neighbour = (x - 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , boþ oda ise = 0
					if (!bObstacleMap[upper_neighbour]) {
						if ((x - 1) == nEndX && y == nEndY) {
							reduceMatrix[self][upper_neighbour] = 100;
						}
						else {
							reduceMatrix[self][upper_neighbour] = 0;
						}
					}
				}

				//Sütun numarasý 0 ý gösterdiðinde sol kýsýmdaki hücreler olmadýðý için y != 0 þartý getirilip sol tarafýn konumuna bakýlmamasý saðlanmýþtýr.
				if (y != 0)
				{
					//Hücrenin sol tarafýna bakýlmasý için yapýlan hesaplama
					int left_neighbour = x * nMapWidth + y - 1;

					//Engel yoksa , hedef nokta ise = 100 , boþ oda ise = 0
					if (!bObstacleMap[left_neighbour]) {
						if (x == nEndX && (y - 1) == nEndY) {
							reduceMatrix[self][left_neighbour] = 100;
						}
						else {
							reduceMatrix[self][left_neighbour] = 0;
						}
					}
				}

				//Sað tarafta kalan en son sütun numarasý olmadýðý sürece yapýlan kontroldür. Burada da sað kýsýmda hücreler olmadýðý için bu koþul konulmuþtur.
				if (y != nMapWidth - 1)
				{
					//Hücrenin sað tarafýna bakýlmasý için yapýlan hesaplama
					int right_neighbour = x * nMapWidth + y + 1;

					//Engel yoksa , hedef nokta ise = 100 , boþ oda ise = 0
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

				//Alt tarafta kalan en son satýr numarasý olmadýðý sürece yapýlan kontroldür. Burada da aþaðý kýsýmda hücreler olmadýðý için bu koþul konulmuþtur.
				if (x != nMapHeight - 1)
				{
					//Hücrenin alt tarafýna bakýlmasý için yapýlan hesaplama
					int lower_neighbour = (x + 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , boþ oda ise = 0
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

		//Terminalde ödül matrisinin görüntülenmesi
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


	//Arayüzden alýnan Ödül matrisi ile global dizi olarak tanýmlanan R matrisinin eþitlenmesi
	bool EqualRmatrix() {

		int sizeReduce = nMapHeight * nMapWidth; //Ödül matrisinin boyutu

		for (int i = 0; i < sizeReduce; i++)
		{
			for (int j = 0; j < sizeReduce; j++)
			{
				R[i][j] = reduceMatrix[i][j];
			}
		}

		return true;
	}


	//Mümkün olan , hareket edebileceði yerlere gidiyor. possible_action[] dizisinde gidebileceði oda numaralarýný tutuyo
	void get_possible_action(double R[100][100], int state, int possible_action[20], int ACTION_NUM) {
		possible_action_num = 0;
		for (int i = 0; i < ACTION_NUM; i++) {
			if (R[state][i] >= 0) {
				possible_action[possible_action_num] = i;
				possible_action_num++;
			}
		}
	}

	//Q tablosunda bulunduðumuz satýrda yani mevcut konumda maksimum deðeri bulup döndürüyor.
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

			// memset possible_action dizi olarak tanýmlýyor.
			memset(possible_action, 0, 10 * sizeof(int)); //10 tane int deðerli dizi tanýmlama her deðer 0
			get_possible_action(R, init_state, possible_action, STATE_NUM);

			next_action = possible_action[rand() % possible_action_num];
			cout << "-- step " << step << ": next action: " << next_action << endl;

			max_q = get_max_q(Q, next_action, STATE_NUM);

			Q_before = Q[init_state][next_action];
			// Formül güncelleme Q(s,a)=R(s,a)+alpha * max{Q(s', a')}
			Q[init_state][next_action] = R[init_state][next_action] + alpha * max_q; //Q learning formulü
			Q_after = Q[init_state][next_action];

			if (next_action == DES_STATE) {
				init_state = rand() % STATE_NUM;
				break;
			}
			else {
				// Hedef konumu bulamadýysam þuan ki konumum yeni denediðim konumdur.
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

		int initial_state = init_state; // Denemeye baþlayacaðý oda numarasý 
		int sizeReduce1 = MATRIX_ROW; //episode_iterator fonksiyonuna yollanmak için tanýmlanmýþtýr.

		// Rastgele baþladýðý durum.
		srand((unsigned)time(NULL));
		cout << "[INFO] start training..." << endl;
		for (int i = 0; i < MAX_EPISODE; ++i) {
			cout << "[INFO] Episode: " << i << endl; //kaç bölümde bulduðunu görmek için her döngüde yazdýrýyor.
			initial_state = episode_iterator(initial_state, Q, R, sizeReduce1, targetRoom);
			cout << "-- updated Q matrix: " << endl;
			print_matrix(Q, MATRIX_ROW, MATRIX_COLUMN);
		}
	}


	//Q learning algoritmasýnýn hesaplanmasýný saðlayan fonksiyon
	bool Qlearning()
	{
		int sizeReduce = nMapHeight * nMapWidth; //Ödül matrisi boyutu

		cout << "Q matris:" << endl;
		print_matrix(Q, sizeReduce, sizeReduce);
		cout << "R matris:" << endl;
		print_matrix(R, sizeReduce, sizeReduce);

		run_training(1, sizeReduce, sizeReduce, targetRoom);

		int startPosition = nStartY * nMapWidth + nStartX; // Baþlangýç konumunun numarasý

		NewCalculate(startPosition); //Arayüzde deðiþtirilen baþlangýç noktasýnýn parametre olarak yollanmasý.
		qLearningDone = true;

		return true;
	}

	//En kýsa yolun gösterilmesi , arayüz üzerinde de en kýsa yol gösterilirken oda numaralarý -1 yapýlmýþtýr.
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
		//Burada her hücrenin kendi numarasýný elde etmek için p fonksiyonu yazýldý.
		auto p = [&](int x, int y) { return y * nMapWidth + x; };
		int size = nMapWidth * nMapHeight; //Boyut.

		//Týklanan kutucuðun X ve Y kordinatlarý.
		int nSelectedCellX = GetMouseX() / nCellSize;
		int nSelectedCellY = GetMouseY() / nCellSize;

		//Sol týk ile týklayýnca engel oluþturulmasý. Engel üstüne bir kez daha týklayýnca engel kalkar.
		if (GetMouse(0).bReleased)
		{
			bObstacleMap[p(nSelectedCellX, nSelectedCellY)] = !bObstacleMap[p(nSelectedCellX, nSelectedCellY)];
		}

		//Baþlangýç durumunun X ve Y kordinatlarý alýnmasý.
		if (GetMouse(1).bReleased)
		{
			nStartX = nSelectedCellX;
			nStartY = nSelectedCellY;

			//Q learning algoritmasý hesaplandýktan sonra baþlangýç konumunun yeni Q learning hesabý için NewCalculate() fonksiyonuna gönderilmesi.
			if (qLearningDone)
			{
				int position = nStartY * nMapWidth + nStartX;
				NewCalculate(position);
			}

		}

		//Hedef durumunun X ve Y kordinatlarýnýn alýnmasý.
		if (GetMouse(2).bReleased)
		{
			nEndX = nSelectedCellY;
			nEndY = nSelectedCellX;

			targetRoom = nEndX * nMapWidth + nEndY; //Hedef noktanýn numarasý.

			qLearningDone = false;
			setReduceMatrix(); //Yeniden hesaplama yapýlacaðý zaman matrislerin deðerlerinin baþlangýç konumuna getirilmesi. 
		}

		//Engeller konulup , baþlangýç ve hedef noktalarý belirlendikten sonra
		//A tuþuna basýldýðýnda ödül matrisinin hesaplanmasý yapýlýr.
		if (GetKey(olc::Key::A).bReleased)
		{
			cout << "--------------------ODUL MATRISI---------------------------" << endl;
			setReduceMatrix();
			calculateReduceMatrix(); //Ödül matrisinin hesaplanmasý.
			EqualRmatrix();

			cout << "------------------------------------------------" << endl;
			string note = "QLearning algoritmasini calistirmeak icin (Q) tusuna basiniz...";
			Menu(note);
		}

		//Q learning algoritmasýnýn hesaplanmasý
		if (GetKey(olc::Key::Q).bReleased)
		{
			Qlearning();
		}

		//Her odanýn numarasýnýn hesaplanmasý (1,2,3,4,5,....)
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

		//Varsayýlan kutucuklarýn mavi renk olmasý.
		olc::Pixel colour = olc::BLUE;

		//Baþlangýç , engel ve hedef noktalarýnýn renklerinin belirlenmesi.
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
				DrawString(x * nCellSize, y * nCellSize, std::to_string(nFlowFieldZ[p(x, y)]), olc::WHITE); //Oda numaralarý gösterilmesi
			}
		}

		return true;
	}
};


int main()
{
	Example demo;

	//Kullanýcýdan satýr , sütun , hedef noktasýnýn alýnmasý , Hedef noktasý burada girildikten sonra arayüzde de deðiþtirilince bir sýkýntý olmuyor.
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

	//Hedef noktasýnýn X ve Y kordinatlarý 
	demo.nEndX = demo.targetRoom / demo.nMapWidth;
	demo.nEndY = demo.targetRoom % demo.nMapWidth;

	string note = " Baslangic, hedef ve engelleri belirledikten sonra \n Odul matrisini olusturmak icin (A) tusuna basiniz.";
	demo.Menu(note);

	cout << endl;

	demo.setReduceMatrix(); //Ödül matrisini boyutlandýrma.

	if (demo.Construct(512, 480, 2, 2))
	{
		demo.Start();
	}

	return 0;
}