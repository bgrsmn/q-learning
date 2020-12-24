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
double Q[100][100]; // 0 değerli güncellenecek
int possible_action[20];
int possible_action_num;


class Example : public olc::PixelGameEngine
{
public:
	int nMapWidth; //Matrisin sütun sayısı
	int nMapHeight; //Matrisin satır sayısı
	int targetRoom; //Hedef nokta.
	int roomValue = 0; //Oda numarası.
	bool qLearningDone = false;
	int nCellSize = 32; //Görünüm için gereken boyut bilgisi
	int nBorderWidth = 4; //Görünüm için gereken boyut bilgisi

	int nStartX; //Başlangıç noktasının X kordinatı
	int nStartY; //Başlangıç noktasının Y kordinatı
	int nEndX;   //Hedef noktasının X kordinatı
	int nEndY;   //Hedef noktasının Y kordinatı

	bool* bObstacleMap; //Default olarak false tanımlanmış engellerin bulunduğu dizi
	int* nFlowFieldZ;
	double** reduceMatrix; //Ödül tablosunu elde edeceğimiz , ödül matrisi

	Example()
	{
		sAppName = "Example";
	}

public:
	bool OnUserCreate() override
	{
		bObstacleMap = new bool[nMapWidth * nMapHeight]{ false }; //Engellerin 1 boyutlu dizi olarak tanımlanması ve -1 değerini alması sağlandı.
		nFlowFieldZ = new int[nMapWidth * nMapHeight]{ 0 }; //Oda numaraları en başta 0 olarak atandı
		return true;
	}

	//Ödül matrisinin dinamik olarak oluşturulması.
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
				reduceMatrix[x][y] = -1.0; //Default olarak her bir değer -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				R[x][y] = -1.0; //Default olarak her bir değer -1 olarak belirlendi.
			}
		}

		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				Q[x][y] = 0; //Default olarak her bir değer 0 olarak belirlendi.
			}
		}

		return true;
	}

	//Terminal üzerinden bilgilendirmelerin yapılması sağlandı.
	void Menu(string note) {

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << " Sol tik ile -> ENGEL , Sag tik ile -> BASLANGIC , Orta Tik ile -> HEDEF" << endl;

		cout << "----------------------------------------------------------------------------------" << endl;

		cout << note << endl;

		cout << "----------------------------------------------------------------------------------" << endl;
	}


	//Ödül matrisinin hesaplanması.
	bool calculateReduceMatrix()
	{
		int size = nMapWidth * nMapHeight; //Boyut.

		//Oluşturulan map'in içerisinde kalmasını ve engeller veya hedef konumuna gelip gelmemesinin kontrolleri yapılmaktadır.
		//Hedef noktası = 100 puan , Engeller = -1 , Başka bir odaya geçebilme durumu = 0
		for (int x = 0; x < nMapHeight; x++)
		{
			for (int y = 0; y < nMapWidth; y++)
			{
				int self = x * nMapWidth + y; //Matristeki her hücrenin kendi numarası elde edilir. (Oda numarası)

				//Satır numarası 0 ı gösterdiğinde üst kısımda hücreler olmadığı için x != 0 şartı getirilip üst konunma bakılmaması sağlanmıştır.
				if (x != 0)
				{
					//Hücrenin yukarısına bakılması için yapılan hesaplama
					int upper_neighbour = (x - 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , boş oda ise = 0
					if (!bObstacleMap[upper_neighbour]) {
						if ((x - 1) == nEndX && y == nEndY) {
							reduceMatrix[self][upper_neighbour] = 100;
						}
						else {
							reduceMatrix[self][upper_neighbour] = 0;
						}
					}
				}

				//Sütun numarası 0 ı gösterdiğinde sol kısımdaki hücreler olmadığı için y != 0 şartı getirilip sol tarafın konumuna bakılmaması sağlanmıştır.
				if (y != 0)
				{
					//Hücrenin sol tarafına bakılması için yapılan hesaplama
					int left_neighbour = x * nMapWidth + y - 1;

					//Engel yoksa , hedef nokta ise = 100 , boş oda ise = 0
					if (!bObstacleMap[left_neighbour]) {
						if (x == nEndX && (y - 1) == nEndY) {
							reduceMatrix[self][left_neighbour] = 100;
						}
						else {
							reduceMatrix[self][left_neighbour] = 0;
						}
					}
				}

				//Sağ tarafta kalan en son sütun numarası olmadığı sürece yapılan kontroldür. Burada da sağ kısımda hücreler olmadığı için bu koşul konulmuştur.
				if (y != nMapWidth - 1)
				{
					//Hücrenin sağ tarafına bakılması için yapılan hesaplama
					int right_neighbour = x * nMapWidth + y + 1;

					//Engel yoksa , hedef nokta ise = 100 , boş oda ise = 0
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

				//Alt tarafta kalan en son satır numarası olmadığı sürece yapılan kontroldür. Burada da aşağı kısımda hücreler olmadığı için bu koşul konulmuştur.
				if (x != nMapHeight - 1)
				{
					//Hücrenin alt tarafına bakılması için yapılan hesaplama
					int lower_neighbour = (x + 1) * nMapWidth + y;

					//Engel yoksa , hedef nokta ise = 100 , boş oda ise = 0
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


	//Arayüzden alınan Ödül matrisi ile global dizi olarak tanımlanan R matrisinin eşitlenmesi
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


	//Mümkün olan , hareket edebileceği yerlere gidiyor. possible_action[] dizisinde gidebileceği oda numaralarını tutuyo
	void get_possible_action(double R[100][100], int state, int possible_action[20], int ACTION_NUM) {
		possible_action_num = 0;
		for (int i = 0; i < ACTION_NUM; i++) {
			if (R[state][i] >= 0) {
				possible_action[possible_action_num] = i;
				possible_action_num++;
			}
		}
	}

	//Q tablosunda bulunduğumuz satırda yani mevcut konumda maksimum değeri bulup döndürüyor.
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

			// memset possible_action dizi olarak tanımlıyor.
			memset(possible_action, 0, 10 * sizeof(int)); //10 tane int değerli dizi tanımlama her değer 0
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
				// Hedef konumu bulamadıysam şuan ki konumum yeni denediğim konumdur.
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

		int initial_state = init_state; // Denemeye başlayacağı oda numarası 
		int sizeReduce1 = MATRIX_ROW; //episode_iterator fonksiyonuna yollanmak için tanımlanmıştır.

		// Rastgele başladığı durum.
		srand((unsigned)time(NULL));
		cout << "[INFO] start training..." << endl;
		for (int i = 0; i < MAX_EPISODE; ++i) {
			cout << "[INFO] Episode: " << i << endl; //kaç bölümde bulduğunu görmek için her döngüde yazdırıyor.
			initial_state = episode_iterator(initial_state, Q, R, sizeReduce1, targetRoom);
			cout << "-- updated Q matrix: " << endl;
			print_matrix(Q, MATRIX_ROW, MATRIX_COLUMN);
		}
	}


	//Q learning algoritmasının hesaplanmasını sağlayan fonksiyon
	bool Qlearning()
	{
		int sizeReduce = nMapHeight * nMapWidth; //Ödül matrisi boyutu

		cout << "Q matris:" << endl;
		print_matrix(Q, sizeReduce, sizeReduce);
		cout << "R matris:" << endl;
		print_matrix(R, sizeReduce, sizeReduce);

		run_training(1, sizeReduce, sizeReduce, targetRoom);

		int startPosition = nStartY * nMapWidth + nStartX; // Başlangıç konumunun numarası

		NewCalculate(startPosition); //Arayüzde değiştirilen başlangıç noktasının parametre olarak yollanması.
		qLearningDone = true;

		return true;
	}

	//En kısa yolun gösterilmesi , arayüz üzerinde de en kısa yol gösterilirken oda numaraları -1 yapılmıştır.
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
		//Burada her hücrenin kendi numarasını elde etmek için p fonksiyonu yazıldı.
		auto p = [&](int x, int y) { return y * nMapWidth + x; };
		int size = nMapWidth * nMapHeight; //Boyut.

		//Tıklanan kutucuğun X ve Y kordinatları.
		int nSelectedCellX = GetMouseX() / nCellSize;
		int nSelectedCellY = GetMouseY() / nCellSize;

		//Sol tık ile tıklayınca engel oluşturulması. Engel üstüne bir kez daha tıklayınca engel kalkar.
		if (GetMouse(0).bReleased)
		{
			bObstacleMap[p(nSelectedCellX, nSelectedCellY)] = !bObstacleMap[p(nSelectedCellX, nSelectedCellY)];
		}

		//Başlangıç durumunun X ve Y kordinatları alınması.
		if (GetMouse(1).bReleased)
		{
			nStartX = nSelectedCellX;
			nStartY = nSelectedCellY;

			//Q learning algoritması hesaplandıktan sonra başlangıç konumunun yeni Q learning hesabı için NewCalculate() fonksiyonuna gönderilmesi.
			if (qLearningDone)
			{
				int position = nStartY * nMapWidth + nStartX;
				NewCalculate(position);
			}

		}

		//Hedef durumunun X ve Y kordinatlarının alınması.
		if (GetMouse(2).bReleased)
		{
			nEndX = nSelectedCellY;
			nEndY = nSelectedCellX;

			targetRoom = nEndX * nMapWidth + nEndY; //Hedef noktanın numarası.

			qLearningDone = false;
			setReduceMatrix(); //Yeniden hesaplama yapılacağı zaman matrislerin değerlerinin başlangıç konumuna getirilmesi. 
		}

		//Engeller konulup , başlangıç ve hedef noktaları belirlendikten sonra
		//A tuşuna basıldığında ödül matrisinin hesaplanması yapılır.
		if (GetKey(olc::Key::A).bReleased)
		{
			cout << "--------------------ODUL MATRISI---------------------------" << endl;
			setReduceMatrix();
			calculateReduceMatrix(); //Ödül matrisinin hesaplanması.
			EqualRmatrix();

			cout << "------------------------------------------------" << endl;
			string note = "QLearning algoritmasini calistirmeak icin (Q) tusuna basiniz...";
			Menu(note);
		}

		//Q learning algoritmasının hesaplanması
		if (GetKey(olc::Key::Q).bReleased)
		{
			Qlearning();
		}

		//Her odanın numarasının hesaplanması (1,2,3,4,5,....)
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

		//Varsayılan kutucukların mavi renk olması.
		olc::Pixel colour = olc::BLUE;

		//Başlangıç , engel ve hedef noktalarının renklerinin belirlenmesi.
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
				DrawString(x * nCellSize, y * nCellSize, std::to_string(nFlowFieldZ[p(x, y)]), olc::WHITE); //Oda numaraları gösterilmesi
			}
		}

		return true;
	}
};


int main()
{
	Example demo;

	//Kullanıcıdan satır , sütun , hedef noktasının alınması , Hedef noktası burada girildikten sonra arayüzde de değiştirilince bir sıkıntı olmuyor.
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

	//Hedef noktasının X ve Y kordinatları 
	demo.nEndX = demo.targetRoom / demo.nMapWidth;
	demo.nEndY = demo.targetRoom % demo.nMapWidth;

	string note = " Baslangic, hedef ve engelleri belirledikten sonra \n Odul matrisini olusturmak icin (A) tusuna basiniz.";
	demo.Menu(note);

	cout << endl;

	demo.setReduceMatrix(); //Ödül matrisini boyutlandırma.

	if (demo.Construct(512, 480, 2, 2))
	{
		demo.Start();
	}

	return 0;
}
