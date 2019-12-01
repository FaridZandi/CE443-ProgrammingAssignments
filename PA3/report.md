تمرین برنامه‌نویسی شبکه ۳ - گزارش تمرین
=======================================


تغییرات ایجاد شده نسبت به طراحی انجام شده به شرح زیر است.

۱. تابع printStateChangeMessage اضافه شده است که وظیفه نمایش پیغام مربوط به تغییر وضعیت در یک interface را دارد:

	void printStateChangeMessage(bgp_state initial_state, bgp_state dest_state, int interface_index);
	
۲. یک آرایه از قفل های
mutex
در نظر گرفته شد تا در مورد متغیر های مشترک بین 
thread 
مربوط به
timer
ها و 
thread
اصلی برنامه 
مشکلی بوجود نیاید. 

	std::mutex* thread_locks
	
۳. شماره 
interface
ای که بسته از آن دریافت شده است را باید به توابعی که هر نوع از بسته ها را مورد بررسی قرار میدادند 
pass 
داد که سهوا فراموش شده بود . 

۴. در timer ها به جای اینکه زمان باقی مانده را نگه داریم 
زمان پایان را نگه میداریم تا دقت کار بالاتر رود. بنابراین 
type
این متغیر ها از
int 
به
time_t
تغییر داده شد. 

	time_t * connect_retry_time_left;
	time_t * hold_timer_time_left;
	time_t * keep_alive_time_left;




