<div dir='rtl'>

# سیستم‌های عامل - تمرین گروهی دوم

## مشخصات گروه

>> نام، نام خانوادگی و ایمیل خود را در ادامه وارد کنید.

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <sba.hashemii@gmail.com> 

امیرمحمد قاسمی <ghasemiamirmohammad@yahoo.com> 

مهرانه نجفی <najafi.mehraneh@gmail.com> 

## مقدمه

>> اگر نکته‌ای درباره فایل‌های سابمیت شده یا برای TAها دارید، لطفا اینجا بیان کنید.

>> اگر از هر منبع برخط یا غیر برخطی به غیر از مستندات Pintos، متن درس، اسلایدهای درس یا نکات گفته شده در کلاس در تمرین گروهی استفاده کرده‌اید، لطفا اینجا آن(ها) را ذکر کنید.

## ساعت زنگ‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در 25 کلمه یا کمتر توضیح بنویسید.

برای حل این مشکل نیازی به تعریف داده‌ساختار جدید حس نشد اما به داده‌ساختار فعلی `thread` یک `ticks` اضافه می‌شود. علت وجود آن جهت تعیین ریسه‌ی مناسب برای آگاه سازی پس از گذشت زمان مشخص شده لازم است.
همچنین یک لیست از ریسه‌های منتظر آگاه سازی در `timer.c` نگه‌داشته می‌شود.

### الگوریتم

>> پرسش دوم: به اختصار آن‌چه هنگام صدا زدن تابع `timer_sleep()` رخ می‌دهد و همچنین اثر `timer interrupt handler` را توضیح دهید.

با صدا زدن تابع `timer_sleep()` ریسه‌ی مورد نظر در لیست ریسه‌های منتظر آگاه‌سازی (در بند بالا) به صورت امن-ریسه قرار می‌گیرد. به این منظور یک تابع `wrapper` تعریف می‌کنیم (برای جزئیات بیشتر پاسخ بخش همگام‌سازی را ببینید) که با کمک آن ترد به لیست یاد‌شده اضافه شده و تابع `thread_block` صدا زده می‌شود. بدین ترتیب تا آگاه سازی منتظر می‌ماند.

>> پرسش سوم: مراحلی که برای کوتاه کردن زمان صرف‌شده در `timer interrupt handler` صرف می‌شود را نام ببرید.

در این بخش با کمک توابع موجود در ساختار `list` کوچکترین ریسه از حیث زمان آگاه‌سازی را بررسی می‌کنیم. در صورتی که از زمان آن گذشته بود آن را با کمک تابع `thread_unblock` آگاه می‌کنیم.

### همگام‌سازی

>> پرسش چهارم: هنگامی که چند ریسه به طور همزمان `timer_sleep()` را صدا می‌زنند، چگونه از `race condition` جلوگیری می‌شود؟

همان‌طور که در بخش قبل گفته شد برای اضافه کردن به لیست از یک تابع `wrapper` استفاده می کنیم. نام این تابع را `thread_push_block` بگذارید.
از طرفی می‌دانیم حالت `race condition` زمانی پیش می‌آید که هم‌زمان دو ریسه در این لیست وارد شوند. با توجه به امن-ریسه نبودن خود تابع ‍`push` ساختار `list`، از تابع تعریف شده استفاده می‌کنیم. به این صورت که ابتدا یک `lock` رو نگه می‌دارد. پوش را انجام می‌دهد سپس تابع `thread_block` را صدا می‌زند و سپس قفل را رها می‌کند.

>> پرسش پنجم: هنگام صدا زدن `timer_sleep()` اگر یک وقفه ایجاد شود چگونه از `race condition` جلوگیری می‌شود؟

حالت `race_condition` در این قسمت یعنی زمان انتظار ریسه آنقدر کوتاه باشد پیش از آنکه وارد لیست مذکور شود زمانش به پایان برسد.
با توجه به نکته‌ای که در بخش پیش گفتیم مشابه آن را برای `pop` هم پیاده‌سازی می‌کنیم. یعنی یک تابع به نام `thread_pop_unblock` تعریف می‌کنیم. که قفل را می‌گیرد و سپس از لیست گفته شده حذف می‌کند و نهایتا تابع `thread_unblock` را صدا می‌زند و نهایتا قفل را رها می‌کند. چون قطعا عملیات `push` پیش از `pop` لازم است می‌توان مطمئن بود برداشتن نزدیک‌ترین ریسه از لیست پس از `thread_block` رخ خواهد داد و نتیجتا تابع `thread_unblock` به درستی صدا زده خواهد شد.

### منطق

>> پرسش ششم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

## زمان‌بند اولویت‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در ۲۵ کلمه یا کمتر توضیح بنویسید.

>> پرسش دوم: داده‌ساختارهایی که برای اجرای `priority donation` استفاده شده‌است را توضیح دهید. (می‌توانید تصویر نیز قرار دهید)

### الگوریتم

>> پرسش سوم: چگونه مطمئن می‌شوید که ریسه با بیشترین اولویت که منتظر یک قفل، سمافور یا `condition variable` است زودتر از همه بیدار می‌شود؟

>> پرسش چهارم: مراحلی که هنگام صدازدن `lock_acquire()` منجر به `priority donation` می‌شوند را نام ببرید. دونیشن‌های تو در تو چگونه مدیریت می‌شوند؟

>> پرسش پنجم: مراحلی که هنگام صدا زدن `lock_release()` روی یک قفل که یک ریسه با اولویت بالا منتظر آن است، رخ می‌دهد را نام ببرید.

### همگام‌سازی

>> پرسش ششم: یک شرایط احتمالی برای رخداد `race condition` در `thread_set_priority` را بیان کنید و توضیح دهید که چگونه پیاده‌سازی شما از رخداد آن جلوگیری می‌کند. آیا می‌توانید با استفاده از یک قفل از رخداد آن جلوگیری کنید؟

### منطق

>> پرسش هفتم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

## سوالات افزون بر طراحی

>> پرسش هشتم: در کلاس سه صفت مهم ریسه‌ها که سیستم عامل هنگامی که ریسه درحال اجرا نیست را ذخیره می‌کند، بررسی کردیم:‍‍ `program counter` ، ‍‍‍`stack pointer` و `registers`. بررسی کنید که این سه کجا و چگونه در `Pintos` ذخیره می‌شوند؟ مطالعه ‍`switch.S` و تابع ‍`schedule` در فایل `thread.c` می‌تواند مفید باشد.

در تابع switch_threads چهار ثبات الزامی برای اجرای ریسه در پشته همان ریسه ذخیره میشود (ثبات های esi, edi, ebp, ebx). این تابع دو ورودی next,cur را میگیرد که ریسه هایی است که قرار است بین آن ها تعویض قرار بگیرد.  خود esp برای ریسه cur در متغیر stack مربوط به شی آن ریسه (object) ذخیره میشود یا به عبارت ساده تر در cur->stack. مقدار esp برای ریسه جدید در next->stack قرار دارد که در این تابع  این مقدار در نهایت در ثبات پردازنده مینشیند. پس از این عمل، ۴ ثبات الزامی ریسه next که در دفعه قبلی که از ریسه next به ریسه دیگری تعویض کردیم در پشته next قرارداده شده است بازیابی میکنیم. بدین ترتیب ریسه next به همان حالتی برمیگررد که زمانی در حال اجرا بوده.
>> پرسش نهم: وقتی یک ریسه‌ی هسته در ‍`Pintos` تابع `thread_exit` را صدا می‌زند، کجا و به چه ترتیبی صفحه شامل پشته و `TCB` یا `struct thread` آزاد می‌شود؟ چرا این حافظه را نمی‌توانیم به کمک صدازدن تابع ‍`palloc_free_page` داخل تابع ‍`thread_exit` آزاد کنیم؟

>> پرسش دهم: زمانی که تابع ‍`thread_tick` توسط `timer interrupt handler` صدا زده می‌شود، در کدام پشته اجرا می‌شود؟

>> پرسش یازدهم: یک پیاده‌سازی کاملا کاربردی و درست این پروژه را در نظر بگیرید که فقط یک مشکل درون تابع ‍`sema_up()` دارد. با توجه به نیازمندی‌های پروژه سمافورها(و سایر متغیرهای به‌هنگام‌سازی) باید ریسه‌های با اولویت بالاتر را بر ریسه‌های با اولویت پایین‌تر ترجیح دهند. با این حال پیاده‌سازی ریسه‌های با اولویت بالاتر را براساس اولویت مبنا `Base Priority` به جای اولویت موثر ‍`Effective Priority` انتخاب می‌کند. اساسا اهدای اولویت زمانی که سمافور تصمیم می‌گیرد که کدام ریسه رفع مسدودیت شود، تاثیر داده نمی‌شود. تستی طراحی کنید که وجود این باگ را اثبات کند. تست‌های `Pintos` شامل کد معمولی در سطح هسته (مانند متغیرها، فراخوانی توابع، جملات شرطی و ...) هستند و می‌توانند متن چاپ کنند و می‌توانیم متن چاپ شده را با خروجی مورد انتظار مقایسه کنیم و اگر متفاوت بودند، وجود مشکل در پیاده‌سازی اثبات می‌شود. شما باید توضیحی درباره این که تست چگونه کار می‌کند، خروجی مورد انتظار و خروجی واقعی آن فراهم کنید.

## سوالات نظرسنجی

پاسخ به این سوالات دلخواه است، اما به ما برای بهبود این درس در ادامه کمک خواهد کرد. نظرات خود را آزادانه به ما بگوئید—این سوالات فقط برای سنجش افکار شماست. ممکن است شما بخواهید ارزیابی خود از درس را به صورت ناشناس و در انتهای ترم بیان کنید.

>> به نظر شما، این تمرین گروهی، یا هر کدام از سه وظیفه آن، از نظر دشواری در چه سطحی بود؟ خیلی سخت یا خیلی آسان؟

>> چه مدت زمانی را صرف انجام این تمرین کردید؟ نسبتا زیاد یا خیلی کم؟

>> آیا بعد از کار بر روی یک بخش خاص از این تمرین (هر بخشی)، این احساس در شما به وجود آمد که اکنون یک دید بهتر نسبت به برخی جنبه‌های سیستم عامل دارید؟

>> آیا نکته یا راهنمایی خاصی وجود دارد که بهتر است ما آنها را به توضیحات این تمرین اضافه کنیم تا به دانشجویان ترم های آتی در حل مسائل کمک کند؟

>> متقابلا، آیا راهنمایی نادرستی که منجر به گمراهی شما شود وجود داشته است؟

>> آیا پیشنهادی در مورد دستیاران آموزشی درس، برای همکاری موثرتر با دانشجویان دارید؟

این پیشنهادات میتوانند هم برای تمرین‌های گروهی بعدی همین ترم و هم برای ترم‌های آینده باشد.

>> آیا حرف دیگری دارید؟

</div>