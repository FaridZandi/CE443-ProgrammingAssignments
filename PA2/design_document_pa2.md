تمرین برنامه‌نویسی شبکه ۲ - مستند طراحی
======================


مقدمات
----------
>> همان‌طور که می‌دانید، علاوه بر کد باید نحوه‌ی عملکر کد و ساختار آن را نیز توضیح دهید. به منظور یکپارچه‌سازی این توضیحات نمونه‌ی زیر را تکمیل نمایید و آن را در موعد مقرر تحویل مستند طراحی تحویل دهید.

اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.



کلاس simpleMachine 
---------------------

از آنجا که این تابع پدر مشترک هر دو کلاس servermachine و clientMachine است، توابع مشترک در این قسمت قرار داده شده است.

>> توابع

	Frame frameFactory(int iface_index, uint16 source_port, uint16_t dest_port, byte data_type, byte message_length,
                       uint32_t dest_ip);
					   
این تابع پارامتر های مختلفی دریافت کرده و قسمت header یک فریم را میسازد. در جاهایی که لازم است، اطلاعات را به ترتیب شبکه تبدیل میکن. 


    static uint16 getDataLength(byte data_type, uint16 message_length);

این تابع بر حسب نوع بسته طول دیتای موجود در آن را برمیگرداند. در صورتی که قصد داشته باشیم message ارسال کنیم طول پیغام ارسالی به عنوان طول دیتا بازگردانده خواهد شد.

    static void setIPChecksum(Frame &frame);

این تابع یک frame را دریافت نموده و قسمت checksum در هدر ip آن بسته را مقدار دهی میکند. 

    static void checkIPChecksum(Frame &frame);

این تابع یک frame را دریافت نموده و بررسی میکند که آیا checksum آن به درستی مقدار دهی شده است یا نه. از این تابع برای تشخیص معتبر بودن بسته استفاده میشود.

    static void decrementTTL(Frame &frame);

این تابع یک بسته را دریافت نموده و از فیلد ttl در هدر ip یک عدد کم میکند. در صورتی که مقدار حاصل برابر با صفر شد، false و در غیر این صورت true برمیگرداند. 

    static bool isFrameMine(Frame &frame);

این تابع مقدار ip مقصد را با مقدار ip مربوط به این node مقایسه میکند تا تشخیص دهد آیا مقصد این بسته همین node بوده است یا نه.

    void routeFrame(Frame &frame);

در صورتی که بسته متعلق به این node نباشد این تابع صدا شده و این تابع بعد از ایجاد تغییرات لازم در بسته مانند کاهش ttl آن را روی خروجی مناسب ارسال خواهد کرد. 

    int findDestinationInterface(uint32 ip);

این تابع همه ی interface های این node را بررسی میکند و بر مبنای ip و mask آن اینترفیس ها  و ip ورودی تشخیص میدهد که این بسته باید روی کدام interface ارسال شود. 
اگر interface مناسبی یافت نشود interface صفر به عنوان شبکه ی عمومی تر به عنوان پاسخ ارسال خواهد شد.

    void sendOutFrame(Frame &frame);
	
این تابع بسته ای را دریافت میکند بر مبنای ip مقصد روی Interface مناسب ارسال میکند. 

    static std::vector<std::string> split(const std::string &s, char delim);

این تابع یک string دریافت کرده و آن را نسبت به کاراکتر ورودی split میکند. 

    static DataType getFrameDataType(Frame& frame);

این تابع نوع بسته را استخراج میکند. 

    static DataType getFrameID(Frame& frame);
	
این تابع id را از بسته استخراج میکند.



کلاس کارخواه 
---------------------
>> داده‌ساختارها

داده‌ساختارهای استفاده‌شده در کد برای ذخیره‌ی اطلاعات را نام برده و علت استفاده‌ی خود را از این داده‌ساختارها شرح دهید. 

	struct peer_data{
		uint32 local_ip;
		uint16 local_port;
		uint32 public_ip;
		uint16 public_port;

		bool ping_sent;
		bool pong_received;

		bool connected_locally;
	};

    std::map<int, peer_data> *peers_data;


در این داده ساختار اطلاعات سایر peer هایی که قصد ارتباط با آنها را داریم را ذخیره میکنیم تا در هنگام ping کردن و یا ارسال پیام  از آن ها برای پر کردن بسته ها استفاده کنیم.

در این جا از داده ساختار map استفاده شده است تا به ازای هر id به راحتی اطلاعات مربوط به آن را به دست بیاوریم. 

برای هر id اطلاعات محلی و  عمومی آن و همچنین این که وضعیت فعلی ارتباط این peer با peer نشان داده شده با id در چه وضعیتی به سر میبرد. 


>> توابع

هر تابعی که قصد استفاده از آن را دارید با ذکر `Prototype` مطرح کنید و در حداکثر دو خط توضیح دهید که این تابع چه ورودی/خروجی دارد و وظیفه‌ی آن چیست. 


    void process_command(std::string command);

این تایع یک string به عنوان ورودی میگیرد و عملیات مربوط به آن را انجام میدهد. 

    void send_request_assigning_id(uint16 port);

این تابع یک بسته از نوع request assiging id ساخته و به server ارسال میکند. 

    void handle_drop_frame(Frame frame);

این تابع در صورت رسیدن بسته ی drop در مبنای Port مبدا در آن بسته تصمیم میگیرد که باید دوباره بسته request assign id را با Port متفاوت ارسال کند یا پیغام مناسبی نمایش دهد. 

    void handle_response_assigning_id_frame(Frame frame);

این تابع هنگام رسیدن بسته response assigning id شماره رسیده را به عنوان id این node ذخیره میکند. 

    void send_request_getting_ip(int id);

این تابع اطلاعات مربوط به یک id را از server میپرسد. 

    void handle_response_getting_ip_frame(Frame frame);

این تابع اطلاعات مربوط به یک id  را در داده ساختار peers_data ذخیره میکنند. 

    void send_request_local_session(int id);
    void send_request_public_session(int id);

این دو تابع بسته ای با محتویات ping ایجاد کرده و آن را به آدرس عمومی یا محلی id وارد شده ارسال میکنند. 

    void handle_request_session_frame(Frame frame);

این تابع بسته ی Ping را دریافت نموده و بسته pong را برای ارسال کننده پس میفرستد.

    void send_message(int id, std::string message);

این تابع پیغام وارد شده در یک بسته وارد میکند و بر مبنای نوع ارتباط ایجاد شده روی مسیر محلی و یا عمومی به id مورد نظر ارسال میکند. 

    void handle_nat_updated_frame(Frame frame);

این تابع بسته nat updated را دریافت کرده و در پاسخ بسته ی request updating info را برای server ارسال میکند. 

    void send_request_updating_info(uint16 port);
	
این تابع بسته ی request updating info را برای server ارسال میکند. 

    void send_status();

این تابع بسته ای از نوع status را برای server ارسال میکند. 

    void handle_status_respond(Frame frame);

این تابع پاسخ بسته status را دریافت کرده و آن را نمایش میدهد. 



>> نحوه‌ی پر کردن بسته‌ها

می‌توانید با ذکر یک مثال بخش‌های مختلف یک بسته‌ی آماده به ارسال را توضیح دهید. 
	
	
ساختار کلی بسته ها به شکل زیر است: 


	struct ethernet_header {
		byte dst[6];
		byte src[6];
		uint16 type;
	} __attribute__ ((packed));

	struct udp_header {
		uint16 source_port;
		uint16 dest_port;
		uint16 length;
		uint16 checksum;
	}__attribute__ ((packed));

	struct frame_header {
		ethernet_header eth;
		iphdr iph;
		udp_header udph;
		byte data_type_id;
	};

	struct packet_data{
		uint32 local_ip;
		uint16 local_port;
		uint32 public_ip;
		uint16 public_port;
	};
	
که از دو قسمت frame header و packet data تشکیل شده است. 

برای مثال در هنگام ارسال بسته  request asssigning id بسته به شکل زیر ساخته خواهد شد:

مبدا ethernet header برابر با مک اینترفیسی است که بسته روی آن ارسال میشود. 
مقصد این هدر برابر خواهد بود با. ff:ff:ff:ff:ff:ff
تایپ ethernet header  برابر با 0x0800‌خواهد بود. 

در قسمت ip همه ی فیلد ها مطابق با صورت تمرین مقدار دهی شده اند 
آدرس گیرنده برابر با 1.1.1.1 است
آدرس فرستنده برابر با ip اینترفیس صفر این node‌است. 
checksum نیز با جمع زدن همه ی داده های ۱۶ بیتی هدر و گرفتن one's complement از حاصل به دست خواهد آمد. 

در قسمت udp پورت مقصد برابر با ۱۲۳۴ خواهد بود 
پورت مبدا نیز برابر با مقدار وارد شده توسط کاربر خواهد بود. 

در نهایت در قسمت دیتا نیز local ip و local port قرار خواهد گرفت. 


کلاس کارگزار
---------------------
>> داده‌ساختارها

داده‌ساختارهای استفاده‌شده در کد برای ذخیره‌ی اطلاعات را نام برده و علت استفاده‌ی خود را از این داده‌ساختارها شرح دهید. 

	struct peer_data{
		uint32 local_ip;
		uint16 local_port;
		uint32 public_ip;
		uint16 public_port;

		bool valid;
	};

    peer_data peers_data[32];
	
	int current_id_index; 


این داده ساختار اطلاعات مربوط به هر کدام از id ها را ذخیره میکند. هر id که اختصاص داده شود با true شدن مقدار valid مشخص خواهد شد. 

id ای که باید به peer بعدی اختصاص داده شود در متغیری مثل current_id_index مشخص میشود. 
	
>> توابع

هر تابعی که قصد استفاده از آن را دارید با ذکر `Prototype` مطرح کنید و در حداکثر دو خط توضیح دهید که این تابع چه ورودی/خروجی دارد و وظیفه‌ی آن چیست. 

    void handle_request_assigning_id(Frame frame);

این تابع یک بسته request assiging id را دریافت نموده و یک id به node موردنظر اختصاص داده، اطلاعات آن را ذخیره میکند و پاسخ را در قالب یک بسته response assiging id به node موردنظر بر میگرداند.

    void send_response_assigning_id(int id, uint32 ip, uint16 port);

این تابع بسته response assiging id را ساخته برای node مورد نظر ارسال میکند.

    void handle_request_getting_ip(Frame frame);

این تابع اطلاعات مربوط به یک id را برای Node مطلوب، در قالب یک بسته ی  response assiging id به طالب ارسال میکند. 

    void send_response_getting_ip(int dest_id, uint32 dest_ip, uint16 dest_port, uint32 local_ip, uint16 local_port,
                                  uint32 public_ip, uint16 public_port);
								  
این تابع یک بسته response getting ip را برای Node مقصد ارسال میکند. 

    void handle_request_updating_info(Frame frame);

این تابع اطلاعات ذخیره شده برای یک id را به روز رسانی میکند. 

    void handle_status(Frame frame);
    void send_respond_status(byte flag, uint32 ip, uint16 port);

این دو تابع بسته status را دریافت میکنند  و وضعیت را در قالب یک بسته  respond status به پرسنده ارسال میکنند. 



>> نحوه‌ی پر کردن بسته‌ها

می‌توانید با ذکر یک مثال بخش‌های مختلف یک بسته‌ی آماده به ارسال را توضیح دهید. 

	
مشابه قسمت clientMachine ، ساختار کلی بسته ها به شکل زیر است: 


	struct ethernet_header {
		byte dst[6];
		byte src[6];
		uint16 type;
	} __attribute__ ((packed));

	struct udp_header {
		uint16 source_port;
		uint16 dest_port;
		uint16 length;
		uint16 checksum;
	}__attribute__ ((packed));

	struct frame_header {
		ethernet_header eth;
		iphdr iph;
		udp_header udph;
		byte data_type_id;
	};

	struct packet_data{
		uint32 local_ip;
		uint16 local_port;
		uint32 public_ip;
		uint16 public_port;
	};
	
که از دو قسمت frame header و packet data تشکیل شده است. 

برای مثال در هنگام ارسال بسته   response asssigning id بسته به شکل زیر ساخته خواهد شد:

مبدا ethernet header برابر با مک اینترفیسی است که بسته روی آن ارسال میشود. 
مقصد این هدر برابر خواهد بود با. ff:ff:ff:ff:ff:ff
تایپ ethernet header  برابر با 0x0800‌خواهد بود. 

در قسمت ip همه ی فیلد ها مطابق با صورت تمرین مقدار دهی شده اند 
آدرس فرستنده برابر با 1.1.1.1 است
آدرس گیرنده نیز برابر با آدرس فرستنده ی بسته ی request assiging id خواهد بود.
checksum نیز با جمع زدن همه ی داده های ۱۶ بیتی هدر و گرفتن one's complement از حاصل به دست خواهد آمد. 

در قسمت udp پورت مبدا برابر با ۱۲۳۴ خواهد بود 
پورت مقصد نیز برابر با مقدار موجود در بسته ی request assiging id خواهد بود.. 
	
در نهایت در قسمت دیتا هم id اختصاص یافته به این node قرار داده خواهد شد. (در اینجا از packet_data استفاده ای نمیشود. )

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



