#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// --- HARDWARE CONFIGURATION ---
#define TXD_PIN 12
#define RXD_PIN 13
#define UART_PORT UART_NUM_2

#define BLINK_PIN GPIO_NUM_5
#define SOLID_PIN GPIO_NUM_6

#define COOLDOWNTIME 4.0f // Seconds
#define JUMP_SCARE_TIME 3.0f // Seconds

void app_main(void) {
    // 1. Setup UART for TF-Mini Plus (115200 Baud)
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_PORT, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // 2. Setup GPIO Output Pins
    gpio_reset_pin(BLINK_PIN);
    gpio_set_direction(BLINK_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SOLID_PIN);
    gpio_set_direction(SOLID_PIN, GPIO_MODE_OUTPUT);

    // 3. Tracking Variables
    uint8_t data[9];
    float prev_distance = -1.0f;
    int64_t prev_calc_time = esp_timer_get_time();
    
    // 4. State Machine Variables
    bool alarm_active = false;
    int64_t safe_start_time = 0; 
    bool is_safe = true;

    // 5. Jump Scare Timeout Variables
    bool was_in_proximity = false;
    int64_t proximity_start_time = 0;

    printf("TF-Mini Plus Radar System Initialized.\n");

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, 9, 20 / portTICK_PERIOD_MS);
        int64_t current_time = esp_timer_get_time();

        if (len == 9 && data[0] == 0x59 && data[1] == 0x59) { // Valid data frame received
            
            float distance = (float)(data[2] + (data[3] << 8));  // Distance in cm
            
            // Print the raw distance every 50th valid cycle
            static int cycle_count = 0; // Only print every 40 cycles to avoid flooding the console
            if (cycle_count % 30 == 0) { 
                printf("Distance: %.0f cm\n", distance);
            }
            cycle_count++; // Increment cycle count for periodic logging
            if (cycle_count >= 1000) { // Reset cycle count after a while to prevent overflow
                cycle_count = 0;
            }

            if (current_time - prev_calc_time >= 100000) {  // Process every 100ms (10Hz)
                bool collision_danger = false;

                // Ensure distance is valid
                if (distance > 0 && distance < 1200) {   // Valid range: 0.1m to 12m
                    
                    // --- THE 6-METER MAXIMUM LIMIT ---
                    if (distance <= 600.0f) {   // Only consider objects within 6 meters

                        // RULE 1: The 3-Second Jump Scare (< 50cm)
                        if (distance <= 50.0f) { 
                            if (!was_in_proximity) {
                                was_in_proximity = true;
                                proximity_start_time = current_time; // Start the 3s clock
                            }
                            
                            // Only trigger if it has been in the zone for LESS than 3 seconds
                            if ((current_time - proximity_start_time) < JUMP_SCARE_TIME * 1000000) {
                                collision_danger = true;
                            }
                        } else {
                            was_in_proximity = false; // Reset if it moves further than 50cm
                        }

                        // RULE 2: Time-To-Collision (Normal Movement)
                        if (prev_distance > 0) { // Calculate approach speed if we have a past reading
                            float dt = (current_time - prev_calc_time) / 1000000.0f; // dt should be ~0.1s
                            float approach_speed = (prev_distance - distance) / dt;  
                            
                            // Check if object is approaching 
                            if (approach_speed > 0.0f) { 
                                float time_to_collision = distance / approach_speed;

                                // ignore if object teleports or is faster than 30kph (833 cm/s)
                                if (approach_speed > 833.0f) {
                                    printf("Ignoring unrealistic approach speed: %.0f cm/s\n", approach_speed);  
                                } else if (time_to_collision < 2.0f) { // Imminent collision within 2 seconds
                                    collision_danger = true;
                                    printf("TTC DANGER! Dist: %.0f cm, Speed: %.0f cm/s\n", distance, approach_speed);
                                }
                            }
                        }
                    } else {
                        // Object is > 600cm. Reset proximity state.
                        was_in_proximity = false; 
                    }
                } else {
                    was_in_proximity = false; // Reset if sensor reads 0 (error/lost)
                }

                // --- ALARM STATE MACHINE (Cooldown) ---
                if (collision_danger) {
                    alarm_active = true;
                    is_safe = false; 
                } else {
                    if (!is_safe) {
                        is_safe = true;
                        safe_start_time = current_time; 
                    } else if ((current_time - safe_start_time) > COOLDOWNTIME * 1000000) { 
                        alarm_active = false; 
                    }
                }
                
                // --- TRACKING UPDATES ---
                if (distance == 0 || distance >= 1200) {
                    prev_distance = -1.0f; 
                } else {
                    prev_distance = distance; 
                }
                
                prev_calc_time = current_time;
            }
        } else {
            // No valid sensor data received. Still handle the cooldown timer.
            int64_t current_time = esp_timer_get_time();
            if (!is_safe) {
                is_safe = true;
                safe_start_time = current_time;
            } else if ((current_time - safe_start_time) > COOLDOWNTIME * 1000000) {
                alarm_active = false;
            }
        }

        // --- HARDWARE PIN CONTROL ---
        if (alarm_active) {
            gpio_set_level(SOLID_PIN, 1); 
            
            // Blink pin 5 every 100ms
            int toggle = (esp_timer_get_time() / 100000) % 2;
            gpio_set_level(BLINK_PIN, toggle);
        } else {
            gpio_set_level(SOLID_PIN, 0); 
            gpio_set_level(BLINK_PIN, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}