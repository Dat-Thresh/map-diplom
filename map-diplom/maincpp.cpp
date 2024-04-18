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


std::mutex MU;//��� cout

//���������� ������� 
template <class T>
class safe_queue {
public:
	~safe_queue() {
		cond_flag.~condition_variable();
	}
	//safe_queue<T>* pop = this;
	std::mutex m;
	std::condition_variable cond_flag;
	std::atomic<bool> end_flag;
public:
	std::queue<T> q;
	//��������� ������� � �������, ������ �����������, ��� ������� �� ����� -- ����� ������
	void q_push(T obj) {
		{
			std::lock_guard<std::mutex> lg(m);			
			q.push(obj);
			MU.lock();
			std::cout << std::this_thread::get_id() << " obj added to queue" << std::endl;
			MU.unlock();
		}
		cond_flag.notify_one();
		
	};
	// ���� �����������, �������� ������ �� ������� �� ���������� + ������� �� �������
	auto q_pop() {
			std::unique_lock<std::mutex> lk(m);

			cond_flag.wait(lk, [this] {return !q.empty() || end_flag.load(); });
			
			//����������� ������� ������ � �������
			if (q.empty() && end_flag.load()) {
				return std::function<void()>([] {});
			}

			auto task = q.front();
			q.pop();
			//std::cout << "obj popped from queue" << std::endl;

			return task;

	}


	bool is_empty() {
		bool buf = q.empty();
		return buf;
	}

	void set_end_flag(bool val) {
		end_flag.store(val);
	}

	void cond_all_notify() {
		cond_flag.notify_all();
	}

	bool get_status_end_flag() {
		return end_flag.load();
	}

};

template <class T>
class thread_pool {
	std::vector<std::thread> vec_th;
	safe_queue<T> sq;
	int th_number; //���������� ����
	int busy_th = 3;//���������� ������� �������
	std::condition_variable end;
	std::mutex mtx;//thread_pool mutex
	
	
	
public:
	//������������� ����� �� ������������ ����� � ��������� ���������� ���������� �������
	~thread_pool() {

		while (true) {
			std::this_thread::sleep_for(200ms);
			if (sq.is_empty()) {
				//sq_flag.store(false);
				//std::cout << "FLAG SET ON FALSE!" << std::endl;
				submit(std::function<void()>([this] { sq.set_end_flag(true); sq.cond_all_notify(); }));
				//vec_th.clear();
				break;
			}
		}
		for (auto& x : vec_th) {
			x.join();
		}
		
		
	}

	thread_pool() {
		//� ������������ ���������� ���������� ����
		th_number = std::thread::hardware_concurrency();
		std::cout << "number of free CORES is: " << th_number-busy_th << std::endl;
		for (int i = 0; i < th_number - busy_th; i++) {
			std::lock_guard<std::mutex>lk(mtx);
			//std::cout << "Start thread with id: " << std::this_thread::get_id() << std::endl;
			vec_th.push_back(std::thread(std::bind(& thread_pool<std::function<void()>>::work, this)));
			sq.set_end_flag(false);
		}
			
	}
	//����������� �������� ������ � ��������� ��, ��������� �� ����� �� �������
	//������� ������, ������� pop �� ��� ���, ���� ������� ��������� �� ��������
	// 
	//����� ��������� condition_variables ����� ��� �������, ������� ��������� ��� ������� 
	// -- ���� ������, ������ ������ -- ��� ����� -- ���������??? -- ����� ���� ������� ���
	void work() {
		while (true) {
			
			auto task = sq.q_pop();
			
			if (sq.get_status_end_flag()) { break; };
			task();

		}
	};
	//�������� ������� � ������. ���������� ��������� ��� std::function ��� std::packaged_task (�� �����!)
	void submit(T obj) {
		sq.q_push(obj);
	};

};

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
		std::thread th1(std::bind(&thread_pool<std::function<void()>>::submit, &th_p, test_f1));
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


	}
	catch (std::exception &er) {
		std::cout << er.what() << std::endl;
	}

	return 0;
}

