تمرین برنامه‌نویسی شبکه ۱ - مستند طراحی
======================


مقدمات
----------
>> همان‌طور که می‌دانید، علاوه بر کد باید نحوه‌ی عملکر کد و ساختار آن را نیز توضیح دهید. به منظور یکپارچه‌سازی این توضیحات نمونه‌ی زیر را تکمیل نمایید و آن را در موعد مقرر تحویل مستند طراحی تحویل دهید.

اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


کلاس SimpleMachine
------------------

>> داده‌ساختارها

	struct ethernet_header {
	    byte dst[6];
	    byte src[6];
	    uint16 type;
	} __attribute__ ((packed));


	struct data_frame {
	    byte type;
	    byte mac[6];
	    byte ip[4];
	    byte time[4];
	} __attribute__ ((packed));
ساختار بسته ها را به شکل این دو struct در نظر میگیریم. 


این کلاس پدر هر دو کلاس ServerMachine و ClientMachine است و توابعی را شامل میشود که توسط هر دو این کلاس مورد استفاده قرار میگیرد.

>> توابع

    void broadcast(Frame frame, int exception = -1);

این تابع بسته ای را دریافت کرده و روی همه ی interface های این ماشین ارسال میکند. میتوان یکی از interface‌ ها از این پروسه مستثنی کرد.


    static std::vector<std::string> split(const std::string &s, char delim);

   این تابع یک string دریافت کرده و آن را به نسبت به کاراکتر فاصله split‌ میکند و vector‌ ای از string ها را برمیگرداند.

    static byte *int_to_byte_network_order(unsigned int value);

این تابع یک مقدار را به صورت integer‌ دریافت میکند و آن را در ترتیب شبکه به صورت آرایه ای از byte ها برمیگرداند.

    static unsigned int byte_network_order_to_int(byte *value);

این تابع یک آرایه از byte ها را در ترتیب شبکه دریافت نموده و آن را به یک integer‌ تبدیل میکند.

    static std::string ip_int_to_string(unsigned int int_ip);

این تابع یک آدرس ip را به صورت integer دریافت میکند و آن را به یک string در قالب dot-decimal‌ تبدیل میکند.

    static unsigned int ip_string_to_int(std::string string_ip);

این تابع یک ip‌ در قالب dot-decimal‌ دریافت کرده و آنرا به یک integer‌ تبدیل میکند.

    static std::string mac_byte_to_string(byte* mac);

این تابع این mac‌ آدرس را در قالب یک آرایه از byte دریافت نموده و آنرا به صورت یک string ‌تبدیل میکند.

    Frame populate_frame(byte data_type, byte *mac, unsigned int ip, unsigned int time);

این تابع اطلاعات لازم برای ساختن یک فریم را دریافت نموده و با تبدیل اطلاعات لازم به ترتیب شبکه، frame ساخته شده را بر میگرداند.

کلاس کارخواه 
---------------------
>> داده‌ساختارها

داده‌ساختارهای استفاده‌شده در کد برای ذخیره‌ی اطلاعات را نام برده و علت استفاده‌ی خود را از این داده‌ساختارها شرح دهید. 

    std::vector<unsigned int> * ips;

شامل همه ی ip هایی است که به این client‌ اختصاص داده شده اند.

    std::vector<std::pair<unsigned int, unsigned int> > * offers;

شامل همه ی ip‌ هایی است که به این client پیشنهاد داده شده است. به ازای هر ip میزان ttl‌‌ آن را نیز نگه میداریم تا در صورتی که زمان درخواستی بیش از آن مقدار بود جلوی گرفتن ip‌ را بگیریم.

    unsigned int extend_request_ip;

هنگامی که client‌ درخواست تمدید یک ip‌ را میکند، آن ip‌ را در اینجا ذخیره میکنیم تا زمانی که پاسخ extend‌ آمد بدانیم باید ip جدید را با کدام ip‌ قدیمی جایگزین کنیم.

>> توابع

    void process_command(std::string command);

این تابع یک دستور را گرفته و آن را اجرا میکند.

    bool is_frame_mine(Frame frame);

این تابع یک frame را میگیرد و تشخیص میدهد که آیا متعلق به این کارخواه هست یا نه.

    void print_all_ips();

این تابع همه ی ip هایی که متعلق به این client‌ هستند را خط به خط نمایش میدهد.

    void send_dhcp_discover(unsigned int time);

این تابع frame‌ ای از نوع DHCPDISCOVER میسازد و آن را روی همه ی interface ها broadcast‌ میکند.

    void handle_hdcp_offer(unsigned int ip, unsigned int time);

این تابع در صورت دریافت یک offer آن را به لیستی از offer هایی که به این client داده شده است اضافه میکند.

    void send_dhcp_request(unsigned int ip, unsigned int time);

این تابع frame ای از نوع DHCPREQUEST‌ میسازد و آن را روی همه ی interface ها broadcast‌ میکند.

    void handle_dhcp_ack(unsigned int ip, unsigned int time);

این تابع ip‌ مشخص شده را به لیست ip‌ های این client اضافه میکند.

    void send_dhcp_release(unsigned int ip);

این تابع frame ای از نوع DHCPRELEASE میسازد و آن را روی همه ی interface ها broadcast‌ میکند. همچنین آن ip را از لیست ip های این client‌ خارج میکند.

    void handle_dhcp_timeout(unsigned int ip);

این تابع ip مشخص شده را از لیست ip های این client خارج میکند.

    void send_request_extend(unsigned int ip, unsigned int time);

این تابع frame ای از نوع REQUEST EXTEND میسازد و آن را روی همه ی interface ها broadcast‌ میکند.

    void handle_response_extend(unsigned int ip, unsigned int time);

این تابع ip قدیمی را با ip جدیدی که تمدید شده است تعویض میکند.

>> نحوه‌ی پر کردن بسته‌ها

می‌توانید با ذکر یک مثال بخش‌های مختلف یک بسته‌ی آماده به ارسال را توضیح دهید. 

- فریم اترنت

	- مک ارسال کننده برابر با مک interface‌ ای است که از آن ارسال میکنیم.
	- مک گیرنده همواره برابر با fff...f است.
	- تایپ همواره برابر با صفر است.

- فریم دیتا

	- تایپ میتواند مقادیر صفر، ۲ ، ۴ و ۶ باشد.
	- مک را همواره برابر با مک Interface شماره صفر قرار میدهیم.
	- آدرس ip بسته به شرایط میتواند صفر باشد(مثلا در حالت discover) و در حالت های دیگر هم ممکن است ip که قرار است عملیاتی روی آن انجام شود را نشان دهد.
	- زمان هم ممکن صفر باشد (مثلا در زمان release) و در حالت های دیگر زمان درخواستی از طرف client‌ را نمایش میدهد.

**لازم به ذکر است که برای فیلد های ip‌ و time‌ لازم است تا این اطلاعات را به ترتیب شبکه تبدیل کنیم و بعد در بسته قرار دهیم.**

کلاس کارگزار
---------------------
>> داده‌ساختارها

داده‌ساختارهای استفاده‌شده در کد برای ذخیره‌ی اطلاعات را نام برده و علت استفاده‌ی خود را از این داده‌ساختارها شرح دهید. 


    std::map<unsigned int, ip_pool_entry> *ip_pool;

    struct ip_pool_entry {
        bool is_allocated;
        bool is_waiting_for_accept;
        byte mac[6];
        unsigned int ttl;
    };


برای هر یک از ip هایی که متعلق به این server هستند، یک سطر در این map ایجاد میکنیم. key در این map‌ برابر با آن ip است و value آن یک struct است که شامل ۴ فیلد مختلف میشود. 
- اول اینکه این ip به کسی اختصاص داده شده است یا نه. 
- دوم اینکه این ip منتظر تایید است از طرف client هست یا نه. ‌
- سوم اینکه مک client‌ ای که این ip را گرفته چیست. 
- چهارم اینکه این ip برای چه مدت زمانی به آن مک اختصاص داده شده است. 


استفاده از map چند مزیت دارد؛
- از آن جا که بر حسب key هایش مرتب میشود،‌ به راحتی میتوان min را در آن یافت و همچنین در هنگام نمایش خود به خود بر حسب ip‌ مرتب شده است. 
- امکان درج key های تکراری در آن نیست و برای یافتن اجتماع subnet های وارد شده مناسب است. 
- به راحتی برای هر ip میتوان به اطلاعات آن دسترسی پیدا کرد. 

>> توابع

هر تابعی که قصد استفاده از آن را دارید با ذکر `Prototype` مطرح کنید و در حداکثر دو خط توضیح دهید که این تابع چه ورودی/خروجی دارد و وظیفه‌ی آن چیست. 

    void process_command(std::string command);

این تابع یک دستور را میگیرد و آن را انجام میدهد. 

    void add_to_pool(std::string prefix_and_mask);

این تابع یک string‌ با فرمت prefix/mask دریافت میکند و همه ی ip‌ های شامل شده را وارد ip_pool میکند.

    void advance_time(unsigned int time);

این تابع زمان را به جلو میبرد و به ترتیب هر ip که expire‌ شد را با ارسال DHCPTIMEOUT‌ مطلع میسازد و آن را به ip_pool باز میگرداند.

    unsigned int get_one_ip(byte* mac, unsigned int ttl);

این تابع کوچک ترین ip‌ موجود در ip_pool را برمیگرداند و تغییرات لازم را در ip_pool_entry اعمال میکند. 

    void print_all_ips();

این تابع همه ی ip‌ های موجود در ip_pool را به صورت مرتب شده نمایش میدهد. 

    void handle_dhcp_discover(int interface, unsigned int time, byte *mac);

این تابع یک ip مناسب یافته و با ارسال یک بسته DHCPOFFER آن را به client‌ میرساند. 

    void send_dhcp_offer(int interface, unsigned int ip, unsigned int time, byte *mac);

این تابع یک بسته DHCPOFFER‌ را تنها روی interface‌ مشخص شده ارسال میکند. 

    void handle_hdcp_request(unsigned int ip, unsigned int time, byte *mac);

این تابع مشخص میکند که این ip‌ به این mac‌ کاملا اختصاص یافته و با ارسال DHCPACK بقیه سرور ها و کلاینت مورد نظر را مطلع میکند. 

    void send_dhcp_ack(int interface, unsigned int ip, unsigned int time, byte *mac);

این تابع یک بسته DHCPACK را تنها روی interface‌ مشخص شده ارسال میکند. 

    void handle_hdcp_ack(unsigned int ip, byte *mac);

این تابع در صورت که این server به این mac یک ip‌ پیشنهاد داده باشد آن را باطل میکند.

    void handle_dhcp_release(unsigned int ip);

این تابع ip مورد نظر را از تملک صاحب فعلی اش خارج مینماید. 

    void send_dhcp_timeout(unsigned int ip, byte *mac);

این تابع یک بسته DHCPTIMEOUT را روی همه ی interface‌ ها ارسال میکند. 

    void handle_request_extend(int interface, unsigned int ip, unsigned int time, byte *mac);

این تابع ip فعلی را از تملک صاحب فعلی آن خارج کرده و با زمان اضافه شده کوچک ترین ip ممکن را با استفاده از یک بسته RESPONSE EXTEND به client مورد نظر اطلاع میدهد. 

    void send_response_extend(int interface, unsigned int ip, unsigned int time, byte *mac);

این تابع یک بسته RESPONSE EXTEND را تنها روی interface‌ مشخص شده ارسال میکند. 


>> نحوه‌ی پر کردن بسته‌ها

می‌توانید با ذکر یک مثال بخش‌های مختلف یک بسته‌ی آماده به ارسال را توضیح دهید. 

- فریم اترنت

	- مک ارسال کننده برابر با مک interface‌ ای است که از آن ارسال میکنیم.
	- مک گیرنده همواره برابر با fff...f است.
	- تایپ همواره برابر با صفر است.

- فریم دیتا

	- تایپ میتواند مقادیر ۱، ۳ ، ۵ و ۷ باشد.
	- مک را همواره برابر با مک Interface شماره صفر client ای که در حال انجام دستوراتش هستیم قرار میدهیم.
	- آدرس ip جدیدی که در اختیاد client قرار داده میشود را شامل میشود. 
	- زمان هم ممکن صفر باشد (مثلا در زمان timeout) و در حالت های دیگر زمان اعتبار ip داده شده را نمایش میدهد.

**لازم به ذکر است که برای فیلد های ip‌ و time‌ لازم است تا این اطلاعات را به ترتیب شبکه تبدیل کنیم و بعد در بسته قرار دهیم.**



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



