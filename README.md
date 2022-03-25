# Adytum
a Linux chat room written in C++

## 安装 Installation

- 克隆此仓库 Clone the repo

  ``` bash
  git clone https://github.com/BambooSlips/Adytum.git  
  ```

- 配置 Configure

  - MySQL 
  
    在”server.cpp “中的第238行配置自己的MySQL数据库；
  
    Configure your own MySQL database in line 238 of  the "server.cpp" ;
  
  - Redis
  
    在"server.cpp"中的第244行配置自己的Redis， 并启动Redis；
  
    Configure your own Redis in line 244 of the "server.cpp" and start Redis;
  
    ```bash
    #在终端中输入 Input in the terminal
    redis-server
    ```
  
- 编译 Compile 

  ``` bash 
  cd ./Adytum
  make
  ```

## 运行 Run   

- 服务器  Server  

  ```bash
  # 两个参数 默认IP为：“127.0.0.1”， 默认端口号为：“8899”
  # The default IP address of the two parameters is "127.0.0.1", and the default port number is "8899"
  ./adytum-server [IP] [端口号]
  ```

- 客户端 Client    

  ```bash
  # 两个参数 默认IP为：“127.0.0.1”， 默认端口号为：“8899”
  # The default IP address of the two parameters is "127.0.0.1", and the default port number is "8899"
  ./adytum-client [IP] [端口号]
  ```

## 例子 Example  

![2022-03-25_18-24](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-24.png)

![2022-03-25_18-29](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-29.png)

![2022-03-25_18-34](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-34.png)

![2022-03-25_18-36_1](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-36_1.png)

![2022-03-25_18-36](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-36.png)

![2022-03-25_18-30](/run/media/breeze/Data/Code/C++/Beyond/Adytum/img/2022-03-25_18-30.png)
