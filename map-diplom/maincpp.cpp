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


//безопасная очередь 
template <class T>
class safe_queue {
	
	safe_queue<T>* pop = this;
	std::mutex m;
	std::condition_variable cond_flag;
	//bool fl = false;
public:
	std::queue<T> q;
	//добавляет заадачу в очередь, ??кидает уведомление, что очередь не пуста -- можно попить
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
	// ждет уведомления, забирает задачу из очереди на реализацию + удаляет из очереди
	void q_pop() {
		std::unique_lock<std::mutex> lk(m);
		//auto check= is_empty;
		std::cout << "queue status " << std::boolalpha << pop->is_empty() << std::endl; // возвращает действительное состояние очереди q
		std::cout << "queue strange status: " << &safe_queue<T>::is_empty << std::endl; //возвращает, что очередь q пуста (true) в любом случае -- почему??
		cond_flag.wait(lk);//как предикат добавить возврат актуального состояния очереди 
	
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
		//в конструкторе запоминаем количество ядер
		auto th_number = std::thread::hardware_concurrency();
		std::cout << "number of CORES is: " << th_number << std::endl;	
		
	}
	//вытаскивает очереднб задачу и выполняет ее, проверяет не пуста ли очередь
	//забрать задачу, вызвать pop
	void work() {
		
		auto task = sq.q_front();
		sq.q_pop();
		task();
	};
	//помещает очередь в задачу. аргументом принимает или std::function или std::packaged_task (на выбор!)
	void submit(T obj) {
		//std::lock_guard<std::mutex> k(mtx);
		sq.q_push(obj);
	};

	//тут будем пулять воркеры в конструктор потоков через вектор потоков, полагаясь на количество ядер доступных-3
	/*void do_work() {
		while ()
	}*/




	~thread_pool() {
		//предусмотреть выход из бесконечного цикла и дожидания завершения выполнения потоков
	}

};
std::mutex MU;

//функция для теста 1
void test_f1() {
	std::this_thread::sleep_for(200ms);
	std::lock_guard<std::mutex> lk(MU);
	std::cout << "th id: " << std::this_thread::get_id() << " " << __FUNCTION__ << " is now working" << std::endl;
}
//функция для теста 2
void test_f2() {
	std::this_thread::sleep_for(200ms);
	std::lock_guard<std::mutex> lk(MU);
	std::cout << "th id: " << std::this_thread::get_id() << " " << __FUNCTION__ << " is working now!" << std::endl;
}

//запускает в двух потоках одновременно сабмит раз в секунду задач объекта thread_pool 

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
		//создаем объект htread_pool 
		thread_pool<std::function<void()>> th_p;
		//th_p.submit(test_f1);

		//в двух потоках  раз в секунду одновременно сабмитим в этото объект наши функции для теста
		put_in_queue(th_p);

		//скорректировать. Пока так, для проверки работы безопасной очереди
		while (true) {
			th_p.work();
		}

	}
	catch (std::exception &er) {
		std::cout << er.what() << std::endl;
	}

	return 0;
}

//воркер подается в конструктор потоков, который реализован на бесконечном цикле кручения вектора потоков, который является полем
//  класса (пока не закончатся задачи)
//что делать с сабмит??
//что-то с очередью?

