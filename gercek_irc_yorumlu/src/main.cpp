#include "Server.hpp"

int Server::port;
int Server::exited = 0;
std::string Server::password;
std::string Server::name = "SKYCHAT";
std::deque<int> Server::fds; //socket fdlerinin(socket tanımlayıcılarının)tutulduğu deque 
std::deque<Channel> Server::channels; // oluşturulan chanellerın tutulduğu deque
std::deque<User> Server::users; // ana socket hariç digerlerinin yani clientlarin tutuldugu deque
sockaddr_in Server::addr;
/*
struct sockaddr_in {
	__uint8_t       sin_len;
	sa_family_t     sin_family;
	in_port_t       sin_port;
	struct  in_addr sin_addr;
	char            sin_zero[8];
};

struct in_addr {
	in_addr_t s_addr;
};

sin_len: Bu alan, yapının toplam uzunluğunu belirtir. Ancak, 
genellikle kullanılmaz ve 0 olarak ayarlanır. BSD tabanlı sistemlerde 
bazen kullanılabilir, ancak genelde sıfırdır.(KULLANMIYORUZ)

sin_family: Bu alan, adres ailesini (address family) belirtir. 
AF_INET olarak ayarlandığında, bu yapı IPv4 adreslerini temsil eder.

sin_port: Bu alan, bağlantı noktası numarasını taşır. Port numarası, 
ağ üzerindeki bir hizmetin belirlenmesinde kullanılır.
HTONS(Host to network short);
16-bitlik bir tamsayı değerini (örneğin, bir port numarası) host (bilgisayar) tarafından kullanılan byte sırasından ağ tarafından kullanılan byte sırasına dönüştürmek için kullanılır.Bu, farklı bilgisayar mimarileri veya ağ protokollerinin farklı byte sıralama (endianness) kurallarını dikkate almayı sağlar.
(bu paragraf çok önemli değil)
//Bilgisayar sistemlerinde iki temel byte sıralama türü bulunur: büyük 
//uçtan küçük uca (big-endian) ve küçük uçtan büyük uca (little-endian).
//Ağ protokollerinin genellikle büyük uçtan küçük uca sıralamayı 
//kullanmasıyla, host tarafındaki bir bilgisayarın byte sıralamasını 
//uygun hale getirmek önemlidir.
Bilgisayarlar, byte'ları farklı sıralarda saklar. Bazıları (big-endian sistemler) en önemli byte'ı (most significant byte) önce saklar, bazıları (little-endian sistemler) ise en az önemli byte'ı (least significant byte) önce saklar. Ancak, ağ protokolleri genellikle big-endian byte sırasını kullanır.
Bu nedenle, bir port numarasını bir soket adres yapısına atarken, htons() fonksiyonunu kullanarak port numarasını host byte sırasından ağ byte sırasına dönüştürmeniz gerekir. Bu, farklı byte sırasına sahip sistemler arasında ağ iletişiminin düzgün çalışmasını sağlar.


sin_addr: Bu alan, struct in_addr tipinde bir yapıdır ve IPv4 adresini 
taşır.
bir sunucu soketinin bağlandığı adresi belirlerken kullanılır ve "INADDR_ANY" sabiti, herhangi bir ağ arayüzünden gelen bağlantıları dinlemek için kullanılır.Bu ifade, sunucunun tüm ağ arabirimlerinden gelen bağlantıları kabul etmesini sağlar.sunucunun bağlandığı adresin "herhangi bir ağ arayüzü" olmasını sağlar. Bu, sunucunun tüm ağ arayüzlerini dinleme konusunda esnek olmasını sağlar ve gelen bağlantıları herhangi bir ağ arayüzünden kabul etmesine izin verir.
Bir IP adresini sin_addr.s_addr'a atamak için, inet_addr() fonksiyonunu kullanabilirsiniz. Bu fonksiyon, bir dize olarak verilen bir IP adresini uygun biçimde dönüştürür. Örneğin:
Server::addr.sin_addr.s_addr = inet_addr("192.168.1.1");
Bu kod satırı, sunucunun yalnızca 192.168.1.1 IP adresinden gelen bağlantıları kabul etmesini sağlar.

sin_zero: Bu alan genellikle kullanılmaz ve sadece boyut dengesi için 
ayrılmıştır.(KULLANMIYORUZ!)

*/
fd_set Server::read_fd_set;

std::string lower(std::string const &s) {
    std::string output;
    for (size_t i = 0; i < s.length(); i++) {
        output += tolower(s[i]);
    }
    return output;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Error: invalid number of arguments !" << std::endl;
        return (1);
    }

    std::string port(argv[1]), password(argv[2]);
    if (port.empty() || password.empty() ||
        port.find_first_not_of("0123456789") != std::string::npos) {
        std::cerr << "Error: invalid arguments !" << std::endl;
        return (1);
    }
    /*
    find_first_not_of fonksiyonu, bir stringin içinde belirtilen karakterler dışında bir karakter bulunduğu ilk konumu döndürür. Eğer belirtilen karakterler dışında bir karakter bulunamazsa, std::string::npos değerini döndürür.
    */

    Server::port = stoi(port);
    Server::password = password;

    Server::createSocket();
    Server::bindSocket();
    std::cout << MAGENTA << "Listening with fd " << Server::getServerSocketFd() << RESET << std::endl;
    Server::selectSocket();

    //buraya ulasmasi icin ana loopdan cikmasi lazim - selectSocket fonksiyonu icindeki while dongusunden cikmasi lazim
    while (Server::users.size() > 0) {
        User &user = Server::users[0];
        user.sendMsg("ERROR :Server is closed\r\n");
        user.closeConnection();
    }
    //dongunun nedeni sunucu kapatildiginda bagli olan tum clientlara sunucunun kapandigini bildirmek

    close(Server::getServerSocketFd());
    //server soketini kapatir
    //socket() fonksiyonu ile oluşturduğunuz bir soketi kullanmayı tamamladığınızda, close() fonksiyonunu kullanarak bu soketi kapatmanız gerekir. Bu, soketin bellekten silinmesini ve sistem kaynaklarının serbest bırakılmasını sağlar.

    std::cout << MAGENTA << "Closing connection " << RESET << std::endl;
    //serverin kapandigini bildirir
    return (0);
}
