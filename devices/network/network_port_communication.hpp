#pragma once

// 头文件
#include <fmt/core.h>
#include <fmt/color.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "devices/serial/uart_serial.hpp"

namespace network_com {

auto idntifier_green = fmt::format(fg(fmt::color::green) | fmt::emphasis::bold, "network");
auto idntifier_red   = fmt::format(fg(fmt::color::red)   | fmt::emphasis::bold, "network");

struct message_pack {
  int num;
  float age;
  char buffer_char[1024];
  message_pack() {
    num = 0;
    age = 0.f;
    memset(buffer_char, 0, sizeof(buffer_char));
  }
};

struct Receive_Data_test_ {
  uint8_t   my_color;
  uint8_t   now_run_mode;
  uint8_t   my_robot_id;
  float   bullet_velocity;

  // Description of the yaw axis angle of the gyroscope (signed)
    float   yaw_angle;
    float   yaw_veloctiy;


  // Description of the pitch axis angle of the gyroscope (signed)
    float   pitch_angle;
    float   pitch_veloctiy;

  Receive_Data_test_() {
    my_color                                   = uart::ALL;
    now_run_mode                               = uart::SUP_SHOOT;
    my_robot_id                                = uart::INFANTRY;
    bullet_velocity                            = 30;
    yaw_angle           = 0.f;
    yaw_veloctiy     = 0.f;
    pitch_angle       = 0.f;
    pitch_veloctiy = 0.f;
  }
};

// 服务端类
class tcp_com {
 public:
  // 构造函数
  tcp_com() = default;
  explicit tcp_com(const int _port, const int _argc);
  explicit tcp_com(char** _argv, const int _argc, std::string _tcp_com_config);
  explicit tcp_com(std::string _tcp_com_config);
  // 析构函数
  ~tcp_com();

  // 初始化服务端
  int initServer();

  /**
   * @brief  接收函数
   * @return int 0:  break
   *             -1: continue
   */
  int recvData();

  // 发送函数
  /**
   * @brief  发送数据
   * @return void 
   */
  void sendData();

  /**
   * @brief  发送数据
   * @param  _yaw             yaw 符号
   * @param  yaw              yaw 绝对值
   * @param  _pitch           pitch 符号
   * @param  pitch            pitch 绝对值
   * @param  depth            深度
   * @param  data_type        是否发现目标
   * @param  is_shooting      开火命令
   */
  void sendData(const int&     _yaw,
                const int16_t& yaw,
                const int&     _pitch,
                const int16_t& pitch,
                const int16_t& _depth,
                const int&     _data_type,
                const int&     _is_shooting);

  /**
   * @brief 发送数据
   *
   * @param _yaw          yaw 数据
   * @param _pitch        pitch 数据
   * @param _depth        深度
   * @param _data_type    是否发现目标
   * @param _is_shooting  开火命令
   */
  void sendData(const float _yaw,   const float _pitch,
                       const int   _depth, const int   _data_type = 0,
                       const int   _is_shooting = 0);

  void sendData(const uart::Write_Data& _write_data);


  // reset 函数
  /**
   * @brief 清空事件备份队列
   */
  void resetEventfdBox();

  /**
   * @brief 返回接受数据的结构体
   * 
   * @return Receive_Data 
   */
  inline uart::Receive_Data returnReceive() { return receive_data_; }
  /**
   * @brief 返回子弹速度
   * 
   * @return int 
   */
  inline int   returnReceiveBulletVelocity() { return receive_data_.bullet_velocity; }
  /**
   * @brief 返回机器人 ID
   * 
   * @return int 
   */
  inline int   returnReceiveRobotId()        { return receive_data_.my_robot_id; }
  /**
   * @brief 返回自身颜色
   * 
   * @return int 
   */
  inline int   returnReceiceColor()          { return receive_data_.my_color; }
  /**
   * @brief 返回模式选择
   * 
   * @return int 
   */
  inline int   returnReceiveMode()           { return receive_data_.now_run_mode; }
  /**
   * @brief 返回陀螺仪 Pitch 轴数据
   * 
   * @return float 
   */
  inline float returnReceivePitch()          { return receive_data_.Receive_Pitch_Angle_Info.pitch_angle; }
  /**
   * @brief 返回陀螺仪 Yaw 轴数据
   * 
   * @return float 
   */
  inline float returnReceiveYaw()                  { return receive_data_.Receive_Yaw_Angle_Info.yaw_angle; }
  /**
   * @brief 返回陀螺仪Yaw轴速度数据
   * 
   * @return float 
   */
  inline float returnReceiveYawVelocity()          { return receive_data_.Receive_Yaw_Velocity_Info.yaw_veloctiy; }
  /**
   * @brief 返回陀螺仪Pitch轴速度数据
   * 
   * @return float 
   */
  inline float returnReceivePitchVelocity()        { return receive_data_.Receive_Pitch_Velocity_Info.pitch_veloctiy;}

 private:
  /* 初始化和配置参数 */
  // argc的大小
  int argc_size = 0;
  // 端口值
  int port = 0;
  // 服务端套接字
  int server_sock;

  // setsockopt相关
  int opt = 1;
  unsigned int opt_len = sizeof(opt);
  // 服务端地址族信息
  struct sockaddr_in server_addr;

  // 读事件的集合，包括监听socket和客户端连接上来的socket。
  fd_set readfdset;
  // readfdset中socket的最大值。
  int maxfd;
  // 临时事件集合
  fd_set tmpfdset;

  // select 返回值
  int infds;

  /* 连接部分 */
  // 客户端地址族信息
  struct sockaddr_in client_addr;
  // 客户端地址族信息长度
  socklen_t client_len = sizeof(client_addr);
  // 客户端套接字
  int client_sock;

  /* 发送和接收部分 */
  // 接受的返回值
  ssize_t isize;
  // 收发的结构体
  // message_pack pack;
  Receive_Data_test_ receive_data_test;
  uart::Receive_Data receive_data_;
  uart::Write_Data   write_data_;

  // 发送所需的文件描述符
  int eventfd_copy;
  std::vector<int> eventfd_box;
};

};  // namespace network_com
