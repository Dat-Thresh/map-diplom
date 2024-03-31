#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

using namespace std::chrono_literals;


//���������� ������� 
template <class T>
class safe_queue {
	
	safe_queue<T>* pop = this;
	std::mutex m;
	std::condition_variable cond_flag;
	//bool fl = false;
public:
	std::queue<T> q;
	//��������� ������� � �������, ??������ �����������, ��� ������� �� ����� -- ����� ������
	void q_push(T obj) {
		{
			std::lock_guard<std::mutex> lg(m);
			
			q.push(obj);
			std::cout << std::this_thread::get_id() << " obj added to queue" << std::endl;
			//std::cout << "queue size: " << q.size() << std::endl;
			
			//lk.unlock();
			//fl = true;
			
		}
		cond_flag.notify_one();
		
	};
	// ���� �����������, �������� ������ �� ������� �� ���������� + ������� �� �������
	void q_pop() {
		std::unique_lock<std::mutex> lk(m);
		//auto check= is_empty;
		std::cout << "queue status " << std::boolalpha << pop->is_empty() << std::endl; // ���������� �������������� ��������� ������� q
		std::cout << "queue strange status: " << &safe_queue<T>::is_empty << std::endl; //����������, ��� ������� q ����� (true) � ����� ������ -- ������??
		cond_flag.wait(lk);//��� �������� �������� ������� ����������� ��������� ������� 
	
		//auto task = q.front();
		q.pop();
		std::cout << "obj popped from queue" << std::endl;	
		
		//return task;
		
	}
	T q_front() {
		std::lock_guard<std::mutex> lg(m);
		if (q.empty()) {
			throw std::runtime_error("queue is empty!");
		}
		return q.front();
	}

	bool is_empty() {
		//std::cout << std::boolalpha << "is_empry checks Queueu is empty?? And it's: " << q.empty() << std::endl;
		bool buf = q.empty();
		return buf;
	}

};

template <class T>
class thread_pool {
	std::vector<std::thread> vec_th;
	safe_queue<T> sq;
	//std::mutex mtx;
	
public:

	thread_pool() {
		//� ������������ ���������� ���������� ����
		auto th_number = std::thread::hardware_concurrency();
		std::cout << "number of CORES is: " << th_number << std::endl;	
		
	}
	//����������� �������� ������ � ��������� ��, ��������� �� ����� �� �������
	//������� ������, ������� pop
	void work() {
		
		auto task = sq.q_front();
		sq.q_pop();
		task();
	};
	//�������� ������� � ������. ���������� ��������� ��� std::function ��� std::packaged_task (�� �����!)
	void submit(T obj) {
		//std::lock_guard<std::mutex> k(mtx);
		sq.q_push(obj);
	};

	//��� ����� ������ ������� � ����������� ������� ����� ������ �������, ��������� �� ���������� ���� ���������-3
	/*void do_work() {
		while ()
	}*/




	~thread_pool() {
		//������������� ����� �� ������������ ����� � ��������� ���������� ���������� �������
	}

};
std::mutex MU;

//������� ��� ����� 1
void test_f1() {
	std::this_thread::sleep_for(200ms);
	std::lock_guard<std::mutex> lk(MU);
	std::cout << "th id: " << std::this_thread::get_id() << " " << __FUNCTION__ << " is now working" << std::endl;
}
//������� ��� ����� 2
void test_f2() {
	std::this_thread::sleep_for(200ms);
	std::lock_guard<std::mutex> lk(MU);
	std::cout << "th id: " << std::this_thread::get_id() << " " << __FUNCTION__ << " is working now!" << std::endl;
}

//��������� � ���� ������� ������������ ������ ��� � ������� ����� ������� thread_pool 

void put_in_queue(thread_pool<std::function<void()>> &th_p) {
	std::mutex m;
	for (int i = 0; i < 5; i++) {
		std::lock_guard<std::mutex> lk(m);
		std::this_thread::sleep_for(100ms);
		std::thread th1(std::bind( &thread_pool<std::function<void()>>::submit, &th_p, test_f1));
		std::thread th2(std::bind(&thread_pool<std::function<void()>>::submit, &th_p, test_f2));
		th1.join();
		th2.join();
	}


}

int main() {
	setlocale(LC_ALL, "rus");


	try {
		//������� ������ htread_pool 
		thread_pool<std::function<void()>> th_p;
		//th_p.submit(test_f1);

		//� ���� �������  ��� � ������� ������������ �������� � ����� ������ ���� ������� ��� �����
		put_in_queue(th_p);

		//���������������. ���� ���, ��� �������� ������ ���������� �������
		while (true) {
			th_p.work();
		}

	}
	catch (std::exception &er) {
		std::cout << er.what() << std::endl;
	}

	return 0;
}

//������ �������� � ����������� �������, ������� ���������� �� ����������� ����� �������� ������� �������, ������� �������� �����
//  ������ (���� �� ���������� ������)
//��� ������ � ������??
//���-�� � ��������?

