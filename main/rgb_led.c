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

    ledc_fade_func_install(0);

}

void setLed(int R, int G, int B) {
    // Clamp the values between 0 and 255
    if (R < 0) R = 0;
    if (R > 255) R = 255;
    if (G < 0) G = 0;
    if (G > 255) G = 255;
    if (B < 0) B = 0;
    if (B > 255) B = 255;

    int curr_R = ledc_get_duty(LEDC_MODE, LEDC_CH0_CHANNEL);
    int curr_G = ledc_get_duty(LEDC_MODE, LEDC_CH1_CHANNEL);
    int curr_B = ledc_get_duty(LEDC_MODE, LEDC_CH2_CHANNEL);

    double diff = color_diff(R, G, B, curr_R, curr_G, curr_B);

    int fade_time = diff * 4;

    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH0_CHANNEL, R, fade_time);
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH1_CHANNEL, G, fade_time);
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CH2_CHANNEL, B, fade_time);

    ledc_fade_start(LEDC_MODE, LEDC_CH0_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE, LEDC_CH1_CHANNEL, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE, LEDC_CH2_CHANNEL, LEDC_FADE_NO_WAIT);

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

    esp_netif_ip_info_t ip_info;

    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1); // 设置ip地址
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0); // 设置子网掩码
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1); // 设置网关

    esp_netif_dhcps_stop(NULL);

    esp_netif_set_ip_info(ap, &ip_info);

    esp_netif_dhcps_start(NULL);

    esp_wifi_start();
}

int create_server_socket() {
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (server_socket < 0) {
        printf("Failed to create socket\n");
        return -1;
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ESP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ESP_IP);

    int bind_result = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if (bind_result < 0) {
        printf("Failed to bind socket\n");
        return -1;
    }

    int listen_result = listen(server_socket, 5);

    if (listen_result < 0) {
        printf("Failed to listen socket\n");
        return -1;
    }

    return server_socket;
}

void receive_command(int client_socket) {
    char buffer[10];

    int recv_len = recv(client_socket, buffer, sizeof(buffer), 0);

    if (recv_len < 0) {
        printf("Failed to receive data\n");
        return;
    }


    // 打印接收到的数据
    printf("Received: %s\n", buffer);

    int *color = get_color(str_to_int(buffer));//返回一个指针，指向R

    setLed(*(color + 0), *(color + 1), *(color + 2));
    printf("R:%d G:%d B:%d", *(color + 0), *(color + 1), *(color + 2));

    free(color);
}

void send_message(int client_socket, const char *message) {
    int message_len = strlen(message);

    int send_len = send(client_socket, message, message_len, 0);

    if (send_len < 0) {
        printf("Failed to send data\n");
        return;
    }

    printf("Sent: %s\n", message);

}

void app_main(void) {
    init_ledc();
    
    init_ap();

    int server_socket = -1;

    while (server_socket < 0) {
        server_socket = create_server_socket();
        if (server_socket < 0)
            printf("Failed to create server socket, retrying...\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while (true) {
        struct sockaddr_in client_addr;

        socklen_t addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &addr_len);

        if (client_socket < 0) {
            printf("Failed to accept client socket\n");
            continue;
        }

        printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        receive_command(client_socket);

        /*// 发送消息到客户端
        const char *message = "Hello from ESP32"; // 消息内容
        send_message(client_socket, message);*/

        close(client_socket);

        printf("Client disconnected\n");

    }

}
