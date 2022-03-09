#include "network_port_communication.hpp"

namespace network_com {
tcp_com::tcp_com(const int _port, const int _argc) {
  port = _port;
  argc_size = _argc;
}

tcp_com::tcp_com(char** _argv, const int _argc, std::string _tcp_com_config) {
  argc_size = _argc;
  if ( argc_size == 2 ) {
  // 读取 argv
  port = atoi(_argv[1]);
  } else if ( argc_size == 1 ) {
    // 读取xml文件
    cv::FileStorage fs_com(_tcp_com_config, cv::FileStorage::READ);

    fs_com["PORT"] >> port;

    fs_com.release();
  } else {
    // 报错
    fmt::print("[{}] usage: ./tcpselect port\n", idntifier_green);
  }
}

tcp_com::tcp_com(std::string _tcp_com_config) {
  // 读取xml文件
  cv::FileStorage fs_com(_tcp_com_config, cv::FileStorage::READ);

  fs_com["PORT"] >> port;

  fs_com.release();
}


tcp_com::~tcp_com() {}

int tcp_com::initServer() {
  // 初始化服务端 socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    fmt::print("[{}] socket() failed.\n", idntifier_green);

    return -1;
  }

  // socket 检查
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len);
  setsockopt(server_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, opt_len);

  // 添加服务端的地址族信息
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // 将服务端地址族信息与 socket 文件描述符进行绑定，并进行检查
  if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    fmt::print("[{}] bind() failed.\n", idntifier_green);

    close(server_sock);
    return -1;
  }

  // 阻塞等待客户端的链接
  if (listen(server_sock, 5) != 0) {
    fmt::print("[{}] listen() failed.\n", idntifier_green);

    close(server_sock);
    return -1;
  }

  fmt::print("[{}] listensock={}\n", idntifier_green, server_sock);

  // 初始化结构体，把listensock添加到集合中。
  FD_ZERO(&readfdset);

  // 将 server_sock 放入 readfdset 队列中，给 select 观察用
  FD_SET(server_sock, &readfdset);
  // 更新文件描述符最大值
  maxfd = server_sock;

  return 1;
}

int tcp_com::recvData() {
  // 调用select函数时，会改变socket集合的内容，所以要把socket集合保存下来，传一个临时的给select。
  tmpfdset = readfdset;

  infds = select(maxfd + 1, &tmpfdset, NULL, NULL, NULL);

  // 返回失败。
  if (infds < 0) {
    fmt::print("[{}] select() failed.\n", idntifier_green);

    perror("select()");
    return -1;
  }

  // 超时，在本程序中，select函数最后一个参数为空，不存在超时的情况，但以下代码还是留着。
  if (infds == 0) {
    fmt::print("[{}] select() timeout.\n", idntifier_green);

    return 0;
  }

  // 检查有事情发生的socket，包括监听和客户端连接的socket。
  // 这里是客户端的socket事件，每次都要遍历整个集合，因为可能有多个socket有事件。

  for (int eventfd = 0; eventfd <= maxfd; eventfd++) {
    if (FD_ISSET(eventfd, &tmpfdset) <= 0) {
      continue;
    }

    if (eventfd == server_sock) {
      // 如果发生事件的是 server_sock，表示有新的客户端连上来。
      client_sock =
          accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
      if (client_sock < 0) {
        fmt::print("[{}] accept() failed.\n", idntifier_green);

        continue;
      }

      fmt::print("[{}] client(socket={}) connected ok.\n", idntifier_green, client_sock);


      // 把新的客户端socket加入集合。
      FD_SET(client_sock, &readfdset);

      if (maxfd < client_sock) maxfd = client_sock;

      continue;

    } else {
      // 保存当前文件描述符
      // eventfd_copy = eventfd;
      eventfd_box.push_back(eventfd);

      // 客户端有数据过来或客户端的socket连接被断开。
      // 清空结构体
      pack = message_pack();

      // 读取客户端的数据。记得改成比赛用接收的结构体
      isize = read(eventfd, &pack, sizeof(pack));

      // 发生了错误或 socket 被对方关闭。
      if (isize <= 0) {
        fmt::print("[{}] client(eventfd={}) disconnected.\n", idntifier_green, eventfd);


        close(eventfd);  // 关闭客户端的socket。

        FD_CLR(eventfd, &readfdset);  // 从集合中移去客户端的socket。

        // 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
        if (eventfd == maxfd) {
          for (int ii = maxfd; ii > 0; ii--) {
            if (FD_ISSET(ii, &readfdset)) {
              maxfd = ii;
              break;
            }
          }

          fmt::print("[{}] maxfd={} \n", idntifier_green, maxfd);
        }

        continue;
      }

      fmt::print("[{}] recv(eventfd= {} ,size= {}): num={} \n", idntifier_green, eventfd, isize, pack.num);
      fmt::print("[{}] recv(eventfd= {} ,size= {}): age={} \n", idntifier_green, eventfd, isize, pack.age);
      fmt::print("[{}] recv(eventfd= {} ,size= {}): buffer_char={} \n", idntifier_green, eventfd, isize, pack.buffer_char);

      // 把收到的报文发回给客户端。
      // write(eventfd,&pack,sizeof(pack));
    }
  }
  return 1;
}

void tcp_com::sendData() {
  for (auto iter = eventfd_box.begin(); iter != eventfd_box.end(); ++iter) {
    write(*iter, &pack, sizeof(pack));
  }

  return;
}

void tcp_com::sendData(const int& _yaw, const int16_t& yaw, const int& _pitch,
                       const int16_t& pitch, const int16_t& _depth,
                       const int& _data_type, const int& _is_shooting) {
  write_data_.symbol_yaw = _yaw;
  write_data_.yaw_angle = yaw;
  write_data_.symbol_pitch = _pitch;
  write_data_.pitch_angle = pitch;
  write_data_.depth = _depth;
  write_data_.data_type = _data_type;
  write_data_.is_shooting = _is_shooting;

  for (auto iter = eventfd_box.begin(); iter != eventfd_box.end(); ++iter) {
    write(*iter, &write_data_, sizeof(write_data_));
  }

  return;
}

void tcp_com::sendData(const float _yaw,   const float _pitch,
                       const int   _depth, const int   _data_type,
                       const int   _is_shooting ) {
  write_data_.symbol_yaw = _yaw >= 0 ? 1 : 0;
  write_data_.yaw_angle = _yaw;
  write_data_.symbol_pitch = _pitch >= 0 ? 1 : 0;
  write_data_.pitch_angle = _pitch;
  write_data_.depth = _depth;
  write_data_.data_type = _data_type;
  write_data_.is_shooting = _is_shooting;

  for (auto iter = eventfd_box.begin(); iter != eventfd_box.end(); ++iter) {
    write(*iter, &write_data_, sizeof(write_data_));
  }

  return;
}

void tcp_com::sendData(const uart::Write_Data& _write_data) {
  write_data_ = _write_data;

  for (auto iter = eventfd_box.begin(); iter != eventfd_box.end(); ++iter) {
    write(*iter, &write_data_, sizeof(write_data_));
  }

  return;
}


void tcp_com::resetEventfdBox() {
  eventfd_box.clear();
  std::vector<int>(eventfd_box).swap(eventfd_box);
}

};  // namespace network_com
