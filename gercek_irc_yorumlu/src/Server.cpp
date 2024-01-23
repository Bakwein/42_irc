#include "Server.hpp"

void Server::createSocket(void) {
    int iSetOption = 1;
    try {
        int sockFd = socket(AF_INET, SOCK_STREAM, 0);
        printf("socket -> %d\n",sockFd);
        if(sockFd == -1)
            throw std::runtime_error("Failed to create socket");
        Server::fds.push_back(sockFd); //ana server socketi dequeya atıldı
        /*
        int sockfd = socket(domain, type, protocol) -> socket üretme fonksiyonu
        -domain->soketin kullanılacağı adres ailesini belirler(40 farklı türü var headerda)
        -type->socket türünü belirler. (6 farklı türü var headerda)
        -protocol -> iletişim protokülünü belirler
        
        #define AF_INET         2   -> internetwork: UDP, TCP, etc. 
        domain:AF_INET -> Tamsayı. iletişim alanını belirtir. IPV4 ile bağlı farklı ana bilgisayarlardaki işlemler arasında iletişim kurmak için AF_INET ve IPV6 ile bağlı işlemler için AF_I NET 6 kullanırız.

        type: iletişim türü
        SOCK_STREAM: TCP (güvenilir, bağlantı odaklı)
        SOCK_DGRAM: UDP (güvenilmez, bağlantısız)
        TCP, güvenilir ve sıralı veri iletimini sağlar, UDP ise daha hafif bir protokol olup güvenilirlik ve sıralama konularında daha esnek bir yaklaşım sunar.

        protocol: İletişim protokolünü belirtir. Bu, belirli bir soket türü için kullanılan altta yatan protokolü belirler. Genellikle 0 olarakayarlanır, bu durumda sistem uygun protokolü seçer. Ancak, belirli bir protokol kullanılması gerektiğinde bu değeri belirtmek gerekir.

        !!!subjectte TCP istiyor o yüzden domain AF_INET, type SOCK_STREAM seçildi.

        protocol parametresine 0 vermek, işletim sisteminin uygun TCP protokolünü seçmesini sağlar.
        */
        if(setsockopt(Server::getServerSocketFd(), SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption)) == -1)
            throw std::runtime_error("Failed to set socket option");
        /*
        int setsockopt(int sockfd, int level, int optname,  const void *optval, socklen_t optlen);
        Bu fonksiyon bir soket üzerindeki seçenekleri ayarlamak için kullanılır.
        Bu, sockfd dosya tanımlayıcısı tarafından yönlendirilen soket için seçenekleri değiştirmeye yardımcı olur. Bu tamamen isteğe bağlıdır, ancak adres ve portun yeniden kullanılmasına yardımcı olur. Aşağıdaki gibi hataları önler: "adres zaten kullanımda".

        -this->server_fd: Bu, seçeneklerin ayarlandığı soketin dosya tanımlayıcısıdır. Bu, soket oluşturulduğunda socket() işlevi tarafından döndürülen dosya tanımlayıcısının aynısıdır.

        -SOL_SOCKET(level number!): Bu, seçeneğin tanımlandığı seviyedir.Bu belirtici, ayarlanacak veya alınacak seçeneğin soket seviyesinde olduğunu belirtir. Yani, bu seçenekler genel soket seçenekleridir ve belirli bir protokol ile ilgili değillerdir.Örneğin, SO_REUSEADDR seçeneği SOL_SOCKET seviyesinde bir seçenektir.

        -optname: Ayarlamak istediğiniz seçeneğin adıdır. Bu, belirli bir seviyede belirli bir soket seçeneğini ifade eder. Örneğin, SO_REUSEADDR seçeneği, aynı adresi ve portu kullanarak birden çok soketin bağlanmasına izin verir.

        -optval: Ayarlamak istediğiniz seçeneğin değerini içeren bir bellek adresidir. Bu, optname ile belirtilen seçeneğin değerini taşır.
        SO_REUSEADDR seçeneği, bir soketin bağlı olduğu adresin (IP ve port) hemen yeniden kullanılabilmesini sağlar. Bu seçeneği etkinleştirmek için, setsockopt fonksiyonuna optval parametresi olarak bir değer sağlamamız gerekiyor.Bu durumda, SO_REUSEADDR seçeneğini etkinleştirmek istiyoruz, bu yüzden optval'a 1 değerini veriyoruz. 1 değeri, bu seçeneğin etkinleştirildiğini belirtir. Eğer 0 değerini verseydik, bu seçenek devre dışı bırakılırdı.

        -optlen: optval belleğinin boyutunu ifade eder.Bu, setsockopt fonksiyonunun optval'da ne kadar bellek okuması gerektiğini bilmesini sağlar.

        */
    } catch (std::exception &e) {
        std::cout << RED << "ERRROR: " << e.what() << RESET << std::endl;
        return;
    }
}

void Server::bindSocket(void) {
    bzero((char *)&Server::addr, sizeof(Server::addr));
    Server::addr.sin_family = AF_INET; // AF_INET, IPv4 adreslerini temsil eder.
    Server::addr.sin_addr.s_addr = INADDR_ANY; //herhangi bir ağ arayüzünden gelen bağlantıları dinlemek için kullanılır.
    Server::addr.sin_port = htons(Server::port); //portu ayarladık.(mainde structın aciklama)
    

    if (bind(Server::getServerSocketFd(), (struct sockaddr *)&Server::addr, sizeof(Server::addr))) {
        std::cout << RED << "ERROR: bind port: " << strerror(errno) << RESET << std::endl;
        exit(1);
    }
    /*
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

    bir soketi belirli bir adres ve portla ilişkilendirmek için kullanılır. Bu, genellikle bir sunucu uygulamasının belirli bir ağ arayüzü ve port üzerinden gelen bağlantıları dinlemesini sağlamak amacıyla kullanılır.

    !!Soket oluşturulduktan sonra bind işlevi, soketi addr'de (özel veri yapısı) belirtilen adrese ve port numarasına bağlar.


    bind fonksiyonu başarılı bir şekilde çalıştığında, 0 değerini döndürür.hata durumu -1

    -sockfd: Bağlanacak soketin dosya tanımlayıcısıdır. Bu, socket fonksiyonu ile oluşturulan bir soketin dosya tanımlayıcısıdır.

    -addr: struct sockaddr tipinde bir yapıdır ve soketin bağlanacağı adres bilgilerini içerir. struct sockaddr yapısı, genellikle struct sockaddr_in (IPv4 için) veya struct sockaddr_in6 (IPv6 için) tipinde bir yapıdır.

    -ddrlen: addr parametresinin boyutunu ifade eder. Bu, kullanılan adres yapısının boyutunu belirtir (sizeof(struct sockaddr_in) veya sizeof(struct sockaddr_in6) gibi).


    */
    if (listen(Server::getServerSocketFd(), FD_SETSIZE)) {
        std::cout << RED << "ERROR: bind port: " << strerror(errno) << RESET << std::endl;
        exit(1);
    }
    /*
    FD_SETSIZE = 1024
    int listen(int sockfd, int backlog);
    
    Sunucu soketini, istemcinin bağlantı kurmak için sunucuya yaklaşmasını beklediği pasif bir moda geçirir. Birikmiş iş, sockfd için bekleyen bağlantı kuyruğunun büyüyebileceği maksimum uzunluğu tanımlar. Kuyruk dolduğunda bir bağlantı isteği gelirse, istemci ECONNREFUSED şeklinde bir hata alabilir.
    
    -this->server_fd:Bu, pasif soket olarak işaretlemek istediğiniz soketin dosya tanımlayıcısıdır.

    -backlog: Bu, bekleyen bağlantılar kuyruğunun maksimum uzunluğudur. Temel olarak, süreç belirli bir bağlantıyı işlerken bekleyebilecek bağlantı sayısıdır. Kuyruk dolduğunda bir bağlantıisteği gelirse, istemci ECONNREFUSED şeklinde bir hata alabilir ya da altta yatan protokol yeniden iletimi destekliyorsa, istek göz ardı edilebilir, böylece daha sonraki bir bağlantı denemesi başarılı olur.
    */
}

void signal_callback_handler(int) {
    Server::exited = 1;
    std::cout << std::endl
              << RED << "=== Good Bye ! ===" << RESET <<  std::endl;
}

void Server::selectSocket(void) {
    int select_output;

    signal(SIGINT, signal_callback_handler); //terminalden ctrl+c geldiğinde sinyal yakalayıp exiti 1 yapıyor.

    //ctrl + c ile gönderilen sinyal yakalanana kadar döner
    while (!Server::exited) {
        printf("dongu yazdirma deneme\n");
        FD_ZERO(&Server::read_fd_set);
        // read_fd_seti sıfırlıyoruz.
        /*
        FD_ZERO
        void FD_ZERO(fd_set *set);
        Bir fd_set yapısını temizlemek için kullanılır. Bu makro, bir fd_set yapısını sıfırlayarak içindeki tüm dosya tanımlayıcılarını (file descriptor) kaldırır ve kullanılamaz hale getirir.
        */
        
        for (size_t i = 0; i < Server::fds.size(); i++) {
            FD_SET(Server::fds[i], &Server::read_fd_set);
            //fdleri read_fd_sete ekliyoruz.
        }
        /*
        FD_SET(int fd, fd_set *set): Belirtilen dosya tanımlayıcısını fd_set yapısına ekler.
        */

        select_output = select(FD_SETSIZE, &Server::read_fd_set, NULL, NULL, NULL);
        //sadece read_fd_sete eklediğimiz fdleri izliyoruz.
        /*
        int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
        
        birden çok dosya tanımlayıcısını (file descriptor) aynı anda izlemek için kullanılır. yani fd'ye attığımız değerleri izliyoruz.

        - `readfds`: Okuma için izlenecek dosya tanımlayıcılarının kümesi. `select()` fonksiyonu döndüğünde, bu küme okumaya hazır olan dosya tanımlayıcılarını içerir.
        - `writefds`: Yazma için izlenecek dosya tanımlayıcılarının kümesi. `select()` fonksiyonu döndüğünde, bu küme yazmaya hazır olan dosya tanımlayıcılarını içerir.
        - `exceptfds`: Hata durumları için izlenecek dosya tanımlayıcılarının kümesi. `select()` fonksiyonu döndüğünde, bu küme hata durumunda olan dosya tanımlayıcılarını içerir.
        - `timeout`: `select()` fonksiyonunun ne kadar süre bekleyeceğini belirler. Bu parametre NULL ise, `select()` fonksiyonu bir dosya tanımlayıcısı hazır olana kadar bekler. “timeout” değeri 0 olarak belirlenmişse, bu genellikle işlemin hemen gerçekleşmesi gerektiği anlamına gelir.Yani, program bir kaynağa veya olaya hemen yanıt vermelidir.

        ### select return degerleri

        1. **Sıfır (`0`):** Belirtilen süre boyunca veya belirtilen olaylar gerçekleşene kadar beklenildi, ancak hiçbir dosya tanımlayıcısı hazır durumda değildi.
        2. **Pozitif Bir Değer (`> 0`):** Geri dönen değer, belirtilen olaylar gerçekleştiği için beklemenin sona erdiğini gösterir. Bu değer, hazır durumda olan toplam dosya tanımlayıcılarının sayısını temsil eder.
        3. **Eksi Bir Değer (`1`):** Fonksiyon bir hata durumunda -1 döner. **`errno`** global değişkeni, oluşan hatanın türünü belirtir.

        */

        if (Server::exited)
            return;

        if (select_output < 0) {
            std::cerr << "Select error" << std::endl;
            return;
        }
        if (FD_ISSET(Server::getServerSocketFd(), &Server::read_fd_set))  //maindeki server soketi fd sette var mı?  
        {

            newConnection(); 
            //BUNU TAM ANLAMADIM - bu sadece 1 kere mi giriyor yani server socketi sadece ilk kısımda oluştuğunda mı giriyor?
            //CEVAP -> selectte set elemanlarından biri eğer set edilirse girdi bekleyen select isleminden ilerler ve alta devam eder buraya da sadece ilk elemanda yani main server soketinde girdi olursa girer. bu da yeni client anlamına gelmektedir
            continue;
        }
        /*
        Sunucu soketi okunmaya hazırsa, bu yeni bir istemcinin bağlanmaya çalıştığı anlamına gelir ve `newConnection` fonksiyonu çağrılır. Bir istemci soketi okunmaya hazırsa, sunucu bu istemciden gelen girdiyi okur.
        */


        /*
        int FD_ISSET(int fd, fd_set *set);

        fd: Kontrol edilmek istenen dosya tanımlayıcısı.
        set: fd'nin içinde bulunduğu fd_set yapısı.
        
        fd_isset fonksiyonu, belirli bir dosya tanımlayıcısının bir olaya hazır olup olmadığını kontrol eder.

        FD_ISSET makrosu, bir fd_set yapısında belirtilen dosya tanımlayıcısının (file descriptor) durumunu kontrol etmek için kullanılır. Bu makro, bir dosya tanımlayıcısının belirli bir fd_set yapısında ayarlı (set) olup olmadığını kontrol eder.

        fd_isset fonksiyonu, belirtilen dosya tanımlayıcısının fd_set yapısındaki ilgili bitin ayarlı (set) olup olmadığını kontrol eder. Eğer ayarlı (set) ise 1 değerini, aksi halde 0 değerini döndürür.

        */

       //selectte set edilen fdlerden biri okunmaya hazırsa bu for döngüsüne girer.

        for (size_t i = 0; i < Server::fds.size(); i++) {
            if (!FD_ISSET(Server::fds[i], &Server::read_fd_set))
                continue;

            for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++) {
                if (it->getFd() == Server::fds[i]) {
                    readInput(*it);
                    break;
                }
            }
        }
    }
}

void Server::newConnection(void) {
    std::cout << RED << "new connect." << RESET << std::endl;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    bzero((char *)&client_addr, addr_len);
    int client_fd = accept(Server::getServerSocketFd(), (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd >= 0) {
        Server::fds.push_back(client_fd);
        Server::users.push_back(User(client_fd));
        std::cout << MAGENTA "New connection with client fd: " << client_fd << RESET << std::endl;
    } else {
        std::cout << RED "ERROR: Failed to connect new client" << RESET << std::endl;
    }
}

void Server::readInput(User &user) {
    int output = 0;
    char buffer[512];  // 512 irc olayından son \r\n olmali

    fcntl(user.getFd(), F_SETFL, O_NONBLOCK); //kullanıcının soketinin dosya tanımlayıcısını bloklamayan moda ayarlamak için fcntl fonksiyonunu kullanır. Bu, recv fonksiyonunun (daha sonra kullanılır) okunacak veri yoksa bloke olmayacağı anlamına gelir; hemen geri döner. O_NONBLOCK kullandık ki blocklanma olmasın tüm fdleri ayarlayabilsin.
    bzero(buffer, 512);
    output = recv(user.getFd(), buffer, 512, 0); // Bu satır, kullanıcının soketinden tampona veri okumak için recv fonksiyonunu kullanır. recv fonksiyonu 512 bayta kadar veri okuyacaktır (tamponun boyutu). recv'nin dönüş değeri (output'ta saklanır) gerçekte okunan bayt sayısıdır veya bir hata oluştuysa -1'dir. Soket bloklama yapmayan modda olduğundan, okunacak veri yoksa recv -1 değerini döndürür ve hata kodunu EAGAIN veya EWOULDBLOCK olarak ayarlar.
    if (output <= 0) 
    { 
        user.closeConnection();
        return;
    }
    std::cout << "buffer: " << buffer << std::endl;
    //0 başka bir client kapatırsa şu ankini kapatırsa -1 hata olursa 
    user.input += buffer;
    //\n'lardan böler
    /*
    /join #channel1
    /join #channel2
    channel1 sağında gizli \n var
    */

    while (user.input.find("\n") != std::string::npos) {
        std::cout << "userrrrr :" << user.input << std::endl;
        std::string cmd = user.input.substr(0, user.input.find("\n"));
        user.input = user.input.substr(user.input.find("\n") + 1); // ilk \nden sonrasini alır
        if (cmd.at(cmd.size() - 1) == '\r')
            cmd = cmd.substr(0, cmd.size() - 1);
        std::cout << "KAMİL: " << cmd << std::endl;
        if (executeCommand(user, cmd)) //true durumunda fonk biter.!!!
            return;
    }
}

// ###Cmd functions###//

void nick(User &user, std::deque<std::string> &cmd) {
    if (cmd.size() < 2) {
        user.sendMsg(":" + Server::name + " 431 " + user.nickName + " :No nickname given\r\n");
        return;
    }
    if (cmd.at(1).find_first_of("#$:") != std::string::npos || isdigit(cmd[1][0]) == true) {
        user.sendMsg(":" + Server::name + " 432 " + user.nickName + " :Invalid nickname\r\n");
        return;
    }
    for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++) {
        if (it->getNickName() == cmd.at(1) && it->getFd() != user.getFd() && user.getUserName().empty()) {
            cmd.at(1) += std::to_string(rand() % 1000);
            user.nickName = cmd.at(1);
        }
        if (it->getNickName() == cmd.at(1) && it->getFd() != user.getFd()) {
            user.sendMsg(":" + Server::name + " 433 " + user.nickName + ":Nickname is already in use\r\n");
            return;
        }
    }
    user.sendMsg(":" + user.nickName + " NICK " + cmd.at(1) + "\r\n");
    user.nickName = cmd.at(1);
}

void sendNoticeMsg(User &user, std::deque<std::string> &cmd, std::string rawcmd) {
    if (cmd.size() == 1) {
        return;
    }
    if (cmd.size() == 2) {
        return;
    }
    if (cmd[2][0] != ':') {
        user.sendMsg("ERROR :Invalid message format\r\n");
        return;
    }
    int i = 0;
    while (rawcmd.at(i) != ':')
        i++;
    cmd.at(2) = rawcmd.substr(i + 1, (rawcmd.size() - i));

    if (cmd[1][0] == '#') {
        for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
            if (it->getName() == lower(cmd.at(1))) {
                for (std::deque<User *>::iterator it2 = it->users.begin(); it2 != it->users.end(); it2++) {
                    if ((*it2)->getFd() == user.fd) {
                        it->sendMsgFromUser(":" + user.nickName + " NOTICE " + cmd.at(1) + " :" + cmd.at(2) + "\r\n", user);
                        return;
                    }
                }
                return;
            }
        }
    } else {
        for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++)
            if (it->getNickName() == cmd.at(1)) {
                it->sendMsg(":" + user.nickName + " NOTICE " + it->nickName + " : " + cmd.at(2) + "\r\n");
                return;
            }
        return;
    }
}

void sendPrivMsg(User &user, std::deque<std::string> &cmd, std::string rawcmd) {
    if (cmd.size() == 1) {
        user.sendMsg("461 " + Server::name + " " + cmd.at(0) + " :Not Enough Parameters\r\n");
        return;
    }
    if (cmd.size() == 2) {
        user.sendMsg(":" + Server::name + " 412 :No text to send\r\n");
        return;
    }
    if (cmd[2][0] != ':') {
        user.sendMsg("ERROR :Invalid message format\r\n");
        return;
    }
    int i = 0;
    while (rawcmd.at(i) != ':')
        i++;
    cmd.at(2) = rawcmd.substr(i + 1, (rawcmd.size() - i));

    if (cmd[1][0] == '#') {
        for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
            if (it->getName() == lower(cmd.at(1))) {
                for (std::deque<User *>::iterator it2 = it->users.begin(); it2 != it->users.end(); it2++) {
                    if ((*it2)->getFd() == user.fd) {
                        it->sendMsgFromUser(":" + user.nickName + " PRIVMSG " + cmd.at(1) + " :" + cmd.at(2) + "\r\n", user);
                        return;
                    }
                }
                user.sendMsg(":" + Server::name + " 404 " + cmd.at(1) + ": Cannot send to CHANNEL\r\n");
                return;
            }
        }
        user.sendMsg(":" + Server::name + " 403 " + cmd.at(1) + ": No such CHANNEL\r\n");
    } else {
        for (std::deque<User>::iterator it = Server::users.begin(); it != Server::users.end(); it++)
            if (it->getNickName() == cmd.at(1)) {
                it->sendMsg(":" + user.nickName + " PRIVMSG " + it->nickName + " : " + cmd.at(2) + "\r\n");
                return;
            }
        user.sendMsg(":" + Server::name + " 401 " + user.nickName + ": No such NICK\r\n");
        return;
    }
}

bool Server::executeCommand(User &user, std::string &cmd) {
    std::cout << BLUE << "[fd: " << user.getFd() << " (" << user.getNickName() << ")] " << RESET << cmd << std::endl;
    std::string stock;
    std::stringstream scmd(cmd);
    std::deque<std::string> cmds;
    while (std::getline(scmd, stock, ' '))  // std::string olan cmd içindeki boşluk karakteri (' ') ile ayrılmış sözcükleri ayıklamak ve bir std::deque<std::string> konteynerine eklemek için kullanılır. 
    {
        cmds.push_back(stock);
        stock.clear();
    }

    if (cmds.at(0) == "CAP")
        return false;
    // ??????????? ne la bu

    else if (cmds.at(0) == "QUIT") {
        user.closeConnection();
        return false;
    }

    else if (cmds.at(0) == "PASS") {
        if (cmds.size() != 2) {
            user.sendMsg("461 " + Server::name + " " + cmds.at(0) + " :Not Enough Parameters\r\n");
            return false;
        }
        if (user.getIsAuth() == true) {
            user.sendMsg("462 " + Server::name + " :Unauthorized command (already registered)\r\n");
            return false;
        }
        if (cmds.at(1) == Server::password)
            user.setIsAuth(true);
        else {
            user.sendMsg("464 " + Server::name + " :Password incorrect\r\n");
            user.closeConnection();
            return true;
        }
    } else if (!user.getIsAuth()) {
        user.sendMsg(":" + Server::name + " 464 " + user.getNickName() + " :You're no authentify !\r\n");
        user.closeConnection();
        return true;
    }

    else if (cmds.at(0) == "PING")
        user.sendMsg("PONG\r\n");

    else if (cmds.at(0) == "NICK")
        nick(user, cmds);

    else if (cmds.at(0) == "USER" && user.getUserName().empty() && user.getNickName().empty() == false) {
        if (cmds.size() < 2) {
            user.sendMsg("461 " + Server::name + " " + cmds.at(0) + " :Not Enough Parameters\r\n");
            return false;
        }
        user.userName = cmds.at(1);
        std::ifstream file("asset/motd.txt");
        std::string text;
        std::string line;
        while (std::getline(file, line))
            text += MAGENTA + line + RESET + "\n";
        user.sendMsg(":" + Server::name + " 001 " + user.nickName + " :" + text + "\r\n");
    } else if (cmds.at(0) == "PRIVMSG" && user.getIsAuth() == true)
        sendPrivMsg(user, cmds, cmd);
    else if (cmds.at(0) == "NOTICE" && user.getIsAuth() == true)
        sendNoticeMsg(user, cmds, cmd);
    else if (cmds.at(0) == "JOIN" && user.getIsAuth() == true) {
        if (cmds.size() < 2) {
            user.sendMsg(":" + user.getNickName() + " 461 :Not Enough Parameters\r\n");
            return false;
        }
        if (cmds.size() > 3 || cmds[1][0] != '#') {
            user.sendMsg(":" + lower(cmds.at(1)) + " 476 :Bad Channel Mask\r\n");
            return false;
        }
        std::string password = cmds.size() == 3 ? cmds.at(2) : "";
        std::string channelName = cmds.at(1);
        user.joinChannel(lower(channelName), true, password);
    } else if (cmds.at(0) == "PART" && user.getIsAuth() == true)
        user.leaveChannel(cmds);
    else if (cmds.at(0) == "KICK" && user.getIsAuth() == true)
        user.kickChannel(cmds, cmd);
    else if (cmds.at(0) == "TOPIC" && user.getIsAuth() == true)
        user.topic(cmds, cmd);
    else if (cmds.at(0) == "INVITE" && user.getIsAuth() == true)
        user.invite(cmds, cmd);
    else if (cmds.at(0) == "MODE" && user.getIsAuth() == true)
        user.mode(cmds, cmd);

    return false;
}

int Server::getServerSocketFd(void) // 0'ı yani server soketin fdsini döndürüyor.
{
    return (Server::fds[0]);
}

void Server::addChannel(std::string const &name, std::string const &operatorName) {
    for (std::deque<Channel>::iterator it = Server::channels.begin(); it != Server::channels.end(); it++) {
        if (it->getName() == name) {
            std::cout << RED << "ERROR: Channel already exist" << RESET << std::endl;
            return;
        }
    }
    Server::channels.push_back(Channel(name, operatorName));
}
