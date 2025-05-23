Kernel ve Core adlı iki farklı dizin oluşturulmasının sebebi;
Aynı anda binlerce paketin gönderilip gelen cevaplar üzerinde
işlem yapmak için gerekli kaynak erişimine high-level OOP
dillerinde sahip değiliz.
Gerçi C/C++ gibi dillerde de değiliz ama,özel spesifik methodlarla
(kernel bypass vb.) bu kaynakları istediğimiz gibi tüketebiliyoruz.
Bu sebepten dolayı natGhost Modülü core ve kernel olmak üzere iki 
main birbirine bağlı kısma ayrılmıştır.
Bu iki dizin, iki programlama dili, arasında API kurulup;high-level 
programming language olan Python ile araştırmalara ve deneylere devam
edilecektir.

natGhost bir proje değil, bir modüldür.
SafeRoom Projesinde karşılaşılan gerçek dünya problemlerini
çözmek üzere zorunluluktan geliştirilen az kaynak kullanarak
en hızlı çözümü tasarımlarını içermektedir.

Tasarlanan pragmatik teknikler araştırma kısmı
bitirilip raporlandırıldıktan sonra Internet Engineering
Task Force a sunulacaktır.

SafeRoom'da natGhost'ta açık kaynaklı yazılımlardır.
Open-Source topluluğunu fanatikçe destekler.
Herhangi mâlî kaygı gütmez,ürün pazarlanabilir ama teknoloji
asla pazarlanamaz.

Burada tasarlanan çözümler recursif olarak
yeni teknolojilerin ve methodların oluşumunu başlatmıştır.

Başta ağ dünyasının üç cisim problemi olarak görünen
ve de Peer to Peer iletişimin şuan gerçek dünyada uygulanmasını
engelleyen Port Redirected Cone NAT ve de özellikle 
delinmesi hem teorikte hemde pratikte imkansız olan
Symmetric Cone NAT ı yüksek oranda delen spesifik methodlar
tasarlanmıştır.

Bu sebepten dolayı natGhost ve SafeRoom 
Peer to Peer iletişimin en büyük sorununu çözmüş olduğundan
sıradan projeler ve modüller değillerdir,bununla beraber
P2P iletişim anlamında yeni teknolojilerin gelişmesini,
gerçek dünyaya entegre edilmesini hızlandıran bir katalizör 
olarak tanımlamak yanlış olmaz.

Uygulanan ve pragmatik çözümler sistem kaynaklarını yüksek oranda
tüketmeyi  gerektirmez tersine,en az kaynakları kullanarak en hızlı 
ve verimli çözüme ulaşır.
ICE Protokolüne göre 10 kat daha hızlı ve de "belirsiz" miktarda daha başarılıdır
bunun nedeni ICE protokolü Public Ip ve Port bulmakta başarılı olsa da
hiçbir zaman Symmetric Cone NAT ı delmekte başarılı olamamıştır.
Ayrıca ICE ın en iyi olduğu kısım olan STUN Server seçme işlemini
natGhost maksimum 100ms de yapar,ICE ise varsayılan olarak 500ms de
tamamlar coğu durumda ise 2-3 retry yaparak 1500ms e kadar bu süre artar
gerçek dünya gözlemlerine göre ise 500ms-1500ms arasında reflexive(STUN) 
adaylarının alınmasıyla başlar,STUN yanıtı ise 50-200 ms arasında döner
bu da seçme sürecini çok daha da uzatır.

Bu süreçlerin hepsini natGhost
ortalama 100ms de başarıyla gerçekleştirir.
En iyi koşullarda en hızlı tamamlanan seçim 29ms
en kötü koşullarda en yavaş tamamlanan seçim 132ms'dir.
En kötü durumda bile ICE ın en iyi durumundan 3.78 kat daha
hızlıdır.
