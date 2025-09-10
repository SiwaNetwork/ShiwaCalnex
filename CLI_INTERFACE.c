// CLI интерфейс для OpenTimeInstrument
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("OpenTimeInstrument CLI Interface\n");
    printf("Использование: ./cli_interface [опции]\n");
    printf("\nДоступные опции:\n");
    printf("  --list-devices, -L    - показать доступные PTP устройства\n");
    printf("  --ptp N              - использовать устройство /dev/ptpN\n");
    printf("  -d device            - использовать указанное устройство\n");
    printf("  -c                   - показать возможности PTP часов\n");
    printf("  -g                   - получить время PTP часов\n");
    printf("  -e N                 - прочитать N событий внешних временных меток\n");
    printf("  -h                   - показать эту справку\n");
    printf("\nПримеры:\n");
    printf("  ./cli_interface --list-devices\n");
    printf("  ./cli_interface --ptp 0 -c\n");
    printf("  ./cli_interface -d /dev/ptp1 -e 100\n");
    
    // Простая логика для демонстрации
    if (argc > 1) {
        if (strcmp(argv[1], "--list-devices") == 0 || strcmp(argv[1], "-L") == 0) {
            printf("\nДоступные PTP устройства:\n");
            system("ls -la /dev/ptp* 2>/dev/null || echo 'PTP устройства не найдены'");
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            // Справка уже показана выше
        } else {
            printf("\nВыполняется команда: ./OpenTimeInstrument");
            for (int i = 1; i < argc; i++) {
                printf(" %s", argv[i]);
            }
            printf("\n");
            
            // Формируем команду для OpenTimeInstrument
            char command[1024] = "./OpenTimeInstrument";
            for (int i = 1; i < argc; i++) {
                strcat(command, " ");
                strcat(command, argv[i]);
            }
            
            // Выполняем команду
            system(command);
        }
    } else {
        printf("\nДля интерактивного режима используйте: ./ptp-cli\n");
    }
    
    return 0;
}