تمرین برنامه‌نویسی شبکه ۳ - مستند طراحی
======================


مقدمات
----------
>> همان‌طور که می‌دانید، علاوه بر کد باید نحوه‌ی عملکر کد و ساختار آن را نیز توضیح دهید. به منظور یکپارچه‌سازی این توضیحات نمونه‌ی زیر را تکمیل نمایید و آن را در موعد مقرر تحویل مستند طراحی تحویل دهید.

اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


کلاس SimulatedMachine 
---------------------
>> داده‌ساختارها

داده‌ساختارهای استفاده‌شده در کد برای ذخیره‌ی اطلاعات را نام برده و علت استفاده‌ی خود را از این داده‌ساختارها شرح دهید.

	
	struct prefix_info{
    	uint32 subnet;
    	short mask;
	};
    std::vector<prefix_info> owned_prefixes;

این داده ساختار برای ذخیره سازی همه prefix هایی است که متعلق به این AS هستند. 
	
	
	struct prefix_route_info{
    	int priority;
    	int interface_index;
    	std::vector<int> path_vector;
	};
    std::map<prefix_info, std::vector<prefix_route_info> > learned_routes;

این داده ساختار برای هر prefix همه مسیر های منتهی به آن را ذخیره مینماید.
برای هر مسیر لیست AS های مسیر به همراه اولویت این مسیر و شماره interface ای که این اطلاعات از آن دریافت شده است را ذخیره میکنیم.
	
	enum peering_mode{
		CUSTOMER,
		PROVIDER,
		PEER
	};
	struct interface_info{
    	peering_mode mode;
    	uint32 peer_ip;
	};
    interface_info * interface_infos;

در این داده ساختار برای هر 
interface 
آدرس آیپی مقصد و همچنین جنس ارتباط را مشخص میکنیم که میتواند از نوع 
peer
یا 
provider
یا 
customer
باشد.

    std::thread ** interface_handler_threads;

برای هر 
interface 
یک 
thread
داریم که مسئول بررسی گذر زمان و ارسال بسته های مناسب و تغییر
state
این
interface
است. 
	
	enum bgp_state{
		IDLE_STATE = 0,
		CONNECT_STATE = 1,
		ACTIVE_STATE = 2,
		OPEN_STATE = 3,
		OPEN_CONFIRM_STATE = 4,
		ESTABLISHED_STATE = 5,
	};
    bgp_state * interface_bgp_states;

در این آرایه
state
هر 
interface 
ذخیره میشود. 


    bool * connect_retry_timer_active;
    int * connect_retry_time_left;

    bool * hold_timer_active;
    int * hold_timer_time_left;

    bool * keep_alive_timer_active;
    int * keep_alive_time_left;
	
	
برای هر 
interface
مشخص خواهیم کرد که کدام
timer
ها در حال حاضر فعال هستند و چه میزان از زمانشان باقی مانده است. 
اگر بخواهیم یک timer را شروع کنیم active را فعال کرده و زمان پایان را هم set میکنیم. thread مربوط در صورت لزوم اقدامات لازم را انجام خواهد داد. 






>> توابع

هر تابعی که قصد استفاده از آن را دارید با ذکر `Prototype` مطرح کنید و در حداکثر دو خط توضیح دهید که این تابع چه ورودی/خروجی دارد و وظیفه‌ی آن چیست. 

	void interface_handler(int interface_index);
	
این تابع در یک 
thread 
جداگانه اجرا میشود و وظیفه بررسی timer ها را دارد. 


	void process_command(string command);
	
این تابع یک خط دستور از ورودی دریافت کرده و عملیات مربوط به آن را اجرا میکند. 

	void startConnection(int iface_index);

این تابع روند اتصال دو 
AS 
را شروع میکند. به این صورت که از حالت 
idle
به حالت 
connect
میرود و بسته 
syn 
پروتکل 
tcp 
را ارسال میکند. 

    void advertiseAll();

این تابع همه ی مسیر های یادگرفته شده را برای سایر 
AS 
های متصل ارسال میکند. 
prefix 
های
own
شده هم در اینجا تبلیغ میشوند. 

    void printRoutesToPrefix(uint32 subnet, int mask);
	
این تابع همه ی مسیر های منتهی به یک 
prefix
را به ترتیب اولویت شان چاپ میکند. 

    void updateInterfacePriority(int interface_index, int priority);
	
این تابع همه ی مسیر های که از این 
interface
شروع میشوند را یافته و اولویت شان را تغییر میدهد. 

    void hijackPrefix(uint32 subnet, int mask);
	
این تابع شروع به تبلیغ این 
prefix
به همسایه های این 
AS
میکند. 

    void withdraw(uint32 subnet, int mask);
	
این تابع 
prefix 
وارد شده را حذف میکند و این اتفاق را در قالب پیام 
update
به اطلاع همسایگان خود میرساند. 

    void handle_tcp_syn_frame(Frame frame);

این تابع در صورت دریافت بسته 
ack
پروتکل
tcp
وارد حالت
open
شده و بسته 
open 
را نیز ارسال مینماید. 

    void handle_open_frame(Frame frame);
	
این تابع مطابق با توضیحات تمرین این 
interface
را از حالت 
openstate 
به 
openconfirm 
میبرد و بسته ی 
keepalive
را ارسال میکند. 

    void handle_update_frame(Frame frame);

این تابع مسیر های حذف شده را حذف میکند و آن را به سایر AS ها منتقل میکند. 

در صورت کشف دور در مسیر تبلیغ شده آن را کنار میگذارد. 

مسیر های جدید را با اولویت متناسب ذخیره میکند . 

علاوه بر این در صورت کشف
hijack 
آن را اعلام میکند. 

    void handle_notification_frame(Frame frame);
	
به حالت 
idle
برمیگردد و پیغام مناسب را نمایش میدهد.

    void handle_keepalive_frame(Frame frame);

این تابع مطابق با توضیحات تمرین ما را از حالت 
openConfirm 
به حالت 
established
میبرد. 

    void fill_ethernet_ip_header(byte* frame_date, uint32 dest_ip, uint32 src_ip, int interface_index);

این تابع هدر های 
ethernet
و 
ip
را پر میکند. 

    uint32 extract_as_num();

این تابع شماره این
AS
را با استفاده از تابع 
getCustomInfo
بدست می آورد. 

    void fill_interface_info(uint32 as_num);

این تابع اطلاعات مربوط به
prefix
های
own 
شده و همچنین اطلاعات مربوط به 
interface
ها را بر مبنای 
as num
پر میکند. 

    std::string getBGPStateName(bgp_state bgp_state);

این تابع عدد مربوط به
bgp state
را تبدیل به 
string 
میکند و برای چاپ پیغام ها کاربرد دارد. 


    static prefix_info prefix_string_to_int(std::string string_prefix);

این تابع یک 
prefix 
را از حالت 
string
به حالت 
integer
تبدیل میکند. 

    static std::string prefix_int_to_string(prefix_info int_prefix);


این تابع یک 
prefix 
را از حالت 
integer
به حالت 
string
تبدیل میکند. 

    std::vector<string> split(string s, char delim);
	
این تابع یک 
string
دریافت کرده و آن را نسبت به یک 
character
میشکند. 

>> نحوه‌ی پر کردن بسته‌ها

می‌توانید با ذکر یک مثال بخش‌های مختلف یک بسته‌ی آماده به ارسال را توضیح دهید. . 

برای مثال یک بسته 
notification 
را بررسی میکنیم. 

در قسمت سرآیند 
ethernet
مقدار 
type 
را برابر با 
0x0800
و مک آدرس ها را برابر با 
FFFFFFFFFFFF
پر میکنیم. 

در قسمت سرآیند آیپی 
مقدار 
version 
را برابر با 4
، 
مقدار
ihl 
را برابر با 
5،
مقدار 
length 
را با 
20 + 19 + 1
که برابر با ۴۰ است 
قرار میدهیم. 
مقدار 
source
و 
dest ip
را مطابق با AS هایی که ارتباط بین شان شکل میگیرد مقدار دهی میکنیم. 
در این جا 
ttl
را برابر با 255 میگذاریم.

در قسمت سرآیند 
BGP
همیشه
۱۲۸ 
بیت 
۱ 
قرار میدهیم. 
type 
را برابر با ۳ میگذاریم ( برای 
notification
). 
همچنین
length 
را هم با 
19 + 1
که برابر با20 است مقدار دهی میکنیم. 


در بسته ی 
notification 
هم یک بایت برای 
error code 
میخواهیم که مثلا ۳ است. 


سرآیند آی‌پی
---------------------
>> تغییر

در صورتی که قصد تفییر در سرآیند آی‌پی به طور مثال در نمونه‌ی جاوا اگر قصد تغییر کلاس `IPv4Header.java` را دارید، نوع تغییر و علت آن را توضیح دهید. 


سرآیند UDP
---------------------
>> تغییر

در صورتی که قصد تفییر در سرآیند آی‌پی به طور مثال در نمونه‌ی جاوا اگر قصد تغییر کلاس `UDP‪.‬java` را دارید، نوع تغییر و علت آن را توضیح دهید. 


سرآیند Ethernet
---------------------
>> تغییر

در صورتی که قصد تفییر در سرآیند آی‌پی به طور مثال در نمونه‌ی جاوا اگر قصد تغییر کلاس `EthernetHeader.java` را دارید، نوع تغییر و علت آن را توضیح دهید. 


کلاس، فایل یا کتابخانه‌های جدید
---------------------
>> افزودن کلاس، فایل یا به‌کارگیری کتابخانه‌های جدید

در صورتی که کلاس یا فایل جدیدی باید اضافه شود یا از کتابخانه‌ی خاصی مورد استفاده قرار بگیرد در این‌جا مطرح کنید. 



