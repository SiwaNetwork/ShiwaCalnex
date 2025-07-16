/*
 * CLI Interface for OpenTimeInstrument
 * Enhanced PTP device selection and management
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_DEVICES 32
#define DEVICE_PATH_LEN 512

struct ptp_device {
    char path[DEVICE_PATH_LEN];
    int number;
    int accessible;
};

// Функция для сканирования доступных PTP устройств
int scan_ptp_devices(struct ptp_device devices[], int max_devices) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    char full_path[DEVICE_PATH_LEN];
    
    dir = opendir("/dev");
    if (dir == NULL) {
        perror("opendir /dev");
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        if (strncmp(entry->d_name, "ptp", 3) == 0) {
            snprintf(full_path, sizeof(full_path), "/dev/%s", entry->d_name);
            
            devices[count].number = atoi(entry->d_name + 3);
            strncpy(devices[count].path, full_path, DEVICE_PATH_LEN - 1);
            devices[count].path[DEVICE_PATH_LEN - 1] = '\0';
            
            // Проверяем доступность устройства
            devices[count].accessible = (access(full_path, R_OK | W_OK) == 0);
            
            count++;
        }
    }
    
    closedir(dir);
    
    // Сортировка по номеру устройства
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (devices[i].number > devices[j].number) {
                struct ptp_device temp = devices[i];
                devices[i] = devices[j];
                devices[j] = temp;
            }
        }
    }
    
    return count;
}

// Функция для вывода списка устройств
void list_ptp_devices() {
    struct ptp_device devices[MAX_DEVICES];
    int count = scan_ptp_devices(devices, MAX_DEVICES);
    
    printf("Доступные PTP устройства:\n");
    printf("%-15s %-12s %s\n", "Устройство", "Номер", "Статус");
    printf("%-15s %-12s %s\n", "----------", "-----", "------");
    
    if (count == 0) {
        printf("Не найдено PTP устройств в системе.\n");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        printf("%-15s %-12d %s\n", 
               devices[i].path, 
               devices[i].number,
               devices[i].accessible ? "Доступно" : "Недоступно (требуются права root)");
    }
    
    printf("\nИспользование:\n");
    printf("  %s --ptp N                    - использовать /dev/ptpN\n", program_invocation_short_name);
    printf("  %s -d /dev/ptpN              - использовать указанное устройство\n", program_invocation_short_name);
}

// Функция для получения пути устройства по номеру
char* get_device_by_number(int ptp_number) {
    static char device_path[DEVICE_PATH_LEN];
    snprintf(device_path, sizeof(device_path), "/dev/ptp%d", ptp_number);
    
    if (access(device_path, F_OK) != 0) {
        fprintf(stderr, "Устройство %s не существует\n", device_path);
        return NULL;
    }
    
    if (access(device_path, R_OK | W_OK) != 0) {
        fprintf(stderr, "Нет доступа к устройству %s (возможно, требуются права root)\n", device_path);
        return NULL;
    }
    
    return device_path;
}

// Расширенная функция usage
void print_enhanced_usage(char *progname) {
    printf("Использование: %s [опции]\n\n", progname);
    
    printf("Опции выбора устройства:\n");
    printf("  --list-devices, -L         показать доступные PTP устройства\n");
    printf("  --ptp N                    использовать устройство /dev/ptpN\n");
    printf("  -d device                  использовать указанное устройство\n\n");
    
    printf("Основные опции:\n");
    printf("  -a val                     запросить одноразовый сигнал через 'val' секунд\n");
    printf("  -A val                     запросить периодический сигнал каждые 'val' секунд\n");
    printf("  -c                         запросить возможности PTP часов\n");
    printf("  -e val                     прочитать 'val' событий внешних временных меток\n");
    printf("  -f val                     скорректировать частоту PTP часов на 'val' ppb\n");
    printf("  -g                         получить время PTP часов\n");
    printf("  -h, --help                 показать эту справку\n");
    printf("  -i val                     индекс для события/триггера\n");
    printf("  -k val                     измерить смещение времени между системой и PHC\n");
    printf("                             'val' раз (максимум 25)\n");
    printf("  -l                         показать текущую конфигурацию пинов\n");
    printf("  -p val                     включить выход с периодом 'val' наносекунд\n");
    printf("  -P val                     включить/выключить (val=1|0) системный PPS\n");
    printf("  -s                         установить время PTP часов из системного времени\n");
    printf("  -S                         установить системное время из PTP часов\n");
    printf("  -t val                     сдвинуть время PTP часов на 'val' секунд\n");
    printf("  -T val                     установить время PTP часов в 'val' секунд\n\n");
    
    printf("Примеры:\n");
    printf("  %s --list-devices          # показать доступные устройства\n", progname);
    printf("  %s --ptp 0 -g              # получить время с /dev/ptp0\n", progname);
    printf("  %s --ptp 1 -c              # показать возможности /dev/ptp1\n", progname);
    printf("  %s -d /dev/ptp0 -s         # синхронизировать PTP часы с системным временем\n", progname);
}

// Основная функция CLI интерфейса
int main(int argc, char *argv[]) {
    char *device = NULL;
    int ptp_number = -1;
    int show_help = 0;
    int list_devices = 0;
    

    
    // Парсинг аргументов командной строки
    // Используем более общий подход - сначала обрабатываем наши специальные опции
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = 1;
            break;
        }
        if (strcmp(argv[i], "--list-devices") == 0 || strcmp(argv[i], "-L") == 0) {
            list_devices = 1;
            break;
        }
        if (strcmp(argv[i], "--ptp") == 0 && i + 1 < argc) {
            ptp_number = atoi(argv[i + 1]);
            // Удаляем эти аргументы из argv для передачи основной программе
            for (int j = i; j < argc - 2; j++) {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
            i--; // Компенсируем сдвиг
        }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            device = argv[i + 1];
            // Удаляем эти аргументы из argv для передачи основной программе
            for (int j = i; j < argc - 2; j++) {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
            i--; // Компенсируем сдвиг
        }
    }
    
    // Обработка команд
    if (show_help) {
        print_enhanced_usage(argv[0]);
        return 0;
    }
    
    if (list_devices) {
        list_ptp_devices();
        return 0;
    }
    
    // Определение устройства для использования
    if (ptp_number >= 0) {
        device = get_device_by_number(ptp_number);
        if (!device) {
            return 1;
        }
    } else if (!device) {
        // Если устройство не указано, попробуем найти первое доступное
        struct ptp_device devices[MAX_DEVICES];
        int count = scan_ptp_devices(devices, MAX_DEVICES);
        
        for (int i = 0; i < count; i++) {
            if (devices[i].accessible) {
                device = devices[i].path;
                printf("Автоматически выбрано устройство: %s\n", device);
                break;
            }
        }
        
        if (!device) {
            fprintf(stderr, "Не найдено доступных PTP устройств. Используйте --list-devices для просмотра.\n");
            return 1;
        }
    }
    
    // Передача управления основной программе
    printf("Используется PTP устройство: %s\n", device);
    
    // Здесь можно добавить вызов основной функциональности OpenTimeInstrument
    // с переданным устройством и остальными аргументами
    
    // Пример построения команды для запуска основной программы:
    char command[1024];
    int pos = snprintf(command, sizeof(command), "./OpenTimeInstrument -d %s", device);
    
    // Добавляем остальные аргументы, если они есть
    for (int i = 1; i < argc; i++) {
        // Экранируем аргументы с пробелами
        if (strchr(argv[i], ' ')) {
            pos += snprintf(command + pos, sizeof(command) - pos, " \"%s\"", argv[i]);
        } else {
            pos += snprintf(command + pos, sizeof(command) - pos, " %s", argv[i]);
        }
    }
    
    printf("Выполняется команда: %s\n", command);
    return system(command);
}
