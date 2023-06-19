#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "color/color_utils.h"

#define RED_PIN 32
#define GREEN_PIN 33
#define BLUE_PIN 25

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_CH2_CHANNEL LEDC_CHANNEL_2

// 定义ESP32的SSID和密码
#define ESP_SSID "Yangser"
#define ESP_PASS "password"

// 定义ESP32的IP地址和端口号
#define ESP_IP "192.168.4.1"
#define ESP_PORT 80

//初始化ledc
void init_ledc() {
    ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_8_BIT, // PWM占空比分辨率
            .freq_hz = 5000,                     // PWM信号的频率
            .speed_mode = LEDC_MODE,             // 定时器模式
            .timer_num = LEDC_TIMER              // 定时器编号
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel[3] = {
            {
                    .channel    = LEDC_CH0_CHANNEL,   //输出pwm的通道
                    .duty       = 0,                  //占空比的值
                    .gpio_num   = RED_PIN,            //gpio端口
                    .speed_mode = LEDC_MODE,          //LEDC通道的速度
                    .timer_sel  = LEDC_TIMER          //设置定时器
            },
            {
                    .channel    = LEDC_CH1_CHANNEL,
                    .duty       = 0,
                    .gpio_num   = GREEN_PIN,
                    .speed_mode = LEDC_MODE,
                    .timer_sel  = LEDC_TIMER
            },
            {
                    .channel    = LEDC_CH2_CHANNEL,
                    .duty       = 0,
                    .gpio_num   = BLUE_PIN,
                    .speed_mode = LEDC_MODE,
                    .timer_sel  = LEDC_TIMER
            }
    };

    for (int ch = 0; ch < 3; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    // 安装渐变函数
    ledc_fade_func_install(0);

}

// 设置具有淡入淡出效果的RGB LED
void setLed(int R, int G, int B) {
    // Clamp the values between 0 and 255
    if (R < 0) R = 0;
    if (R > 255) R = 255;
    if (G < 0) G = 0;
    if (G > 255) G = 255;
    if (B < 0) B = 0;
    if (B > 255) B = 255;

    // Get the current duty values for each channel
    int curr_R = ledc_get_duty(LEDC_MODE, LEDC_CH0_CHANNEL);
    int curr_G = ledc_get_duty(LEDC_MODE, LEDC_CH1_CHANNEL);
    int curr_B = ledc_get_duty(LEDC_MODE, LEDC_CH2_CHANNEL);

    // Calculate the color difference from the current color
    double diff = color_diff(R, G, B, curr_R, curr_G, curr_B);

    // Set the fade time based on the color difference
    int fade_time = diff * 4;

    // Set the fade parameters for each channel
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH0_CHANNEL, R, fade_time);
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH1_CHANNEL, G, fade_time);
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH2_CHANNEL, B, fade_time);

    // Start fading for each channel
    ledc_fade_start(LEDC_MODE, LEDC_CH0_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE, LEDC_CH1_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE, LEDC_CH2_CHANNEL, LEDC_FADE_NO_WAIT);

    // Wait for the fade to finish
    vTaskDelay(fade_time / portTICK_PERIOD_MS);
}

// 初始化ESP32的AP模式
void init_ap() {

    nvs_flash_init();

    esp_netif_init();

    esp_event_loop_create_default();

    esp_netif_t *ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t ap_config = {
            .ap = {
                    .ssid = ESP_SSID,
                    .ssid_len = strlen(ESP_SSID),
                    .password = ESP_PASS,
                    .max_connection = 4,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK
            }
    };

    esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);

    // 创建一个ip信息结构体
    esp_netif_ip_info_t ip_info;

    // 设置ip信息结构体的参数
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1); // 设置ip地址
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0); // 设置子网掩码
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1); // 设置网关

    esp_netif_dhcps_stop(NULL);

    esp_netif_set_ip_info(ap, &ip_info);

    esp_netif_dhcps_start(NULL);

    esp_wifi_start();
}

// 创建一个TCP服务器socket并监听客户端连接请求
int create_server_socket() {
    // 创建一个socket描述符
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    // 检查socket是否创建成功
    if (server_socket < 0) {
        printf("Failed to create socket\n");
        return -1;
    }

    // 创建一个地址结构体
    struct sockaddr_in server_addr;

    // 设置地址结构体的参数
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ESP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ESP_IP);

    // 绑定socket和地址
    int bind_result = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));

    // 检查绑定是否成功
    if (bind_result < 0) {
        printf("Failed to bind socket\n");
        return -1;
    }

    // 监听socket
    int listen_result = listen(server_socket, 5);

    // 检查监听是否成功
    if (listen_result < 0) {
        printf("Failed to listen socket\n");
        return -1;
    }

    // 返回socket描述符
    return server_socket;
}

// 接收客户端的命令并控制LED灯的开关
void receive_command(int client_socket) {
    // 创建一个缓冲区
    char buffer[10];

    // 接收客户端发送的数据
    int recv_len = recv(client_socket, buffer, sizeof(buffer), 0);

    // 检查接收是否成功
    if (recv_len < 0) {
        printf("Failed to receive data\n");
        return;
    }

    // 如果不同，说明有新的颜色更新，中断setLed函数



    // 打印接收到的数据
    printf("Received: %s\n", buffer);

    int *color = get_color(str_to_int(buffer));//返回一个指针，指向R

    setLed(*(color + 0), *(color + 1), *(color + 2));
    printf("R:%d G:%d B:%d", *(color + 0), *(color + 1), *(color + 2));

    free(color); //释放内存
}

// 发送消息到客户端
void send_message(int client_socket, const char *message) {
    // 获取消息的长度
    int message_len = strlen(message);

    // 发送消息到客户端
    int send_len = send(client_socket, message, message_len, 0);

    // 检查发送是否成功
    if (send_len < 0) {
        printf("Failed to send data\n");
        return;
    }

    // 打印发送的数据
    printf("Sent: %s\n", message);

}

void app_main(void) {
    //初始化ledc
    init_ledc();

    // 初始化AP模式
    init_ap();

    // 创建一个服务器socket
    int server_socket = -1;

    // 循环尝试创建socket，直到成功
    while (server_socket < 0) {
        server_socket = create_server_socket();
        if (server_socket < 0)
            printf("Failed to create server socket, retrying...\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while (true) {
        // 创建一个地址结构体
        struct sockaddr_in client_addr;

        // 创建一个地址长度变量
        socklen_t addr_len = sizeof(client_addr);

        // 接受客户端的连接请求
        int client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &addr_len);

        // 检查客户端socket是否创建成功
        if (client_socket < 0) {
            printf("Failed to accept client socket\n");
            continue;
        }

        // 打印客户端的IP地址和端口号
        printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 接收客户端的命令并控制LED灯的开关
        receive_command(client_socket);

        /*// 发送消息到客户端
        const char *message = "Hello from ESP32"; // 消息内容
        send_message(client_socket, message);*/

        // 关闭客户端socket
        close(client_socket);

        // 打印客户端已断开连接的信息
        printf("Client disconnected\n");

    }

}