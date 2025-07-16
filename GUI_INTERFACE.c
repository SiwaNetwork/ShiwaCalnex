#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>

static GtkWidget *device_entry;
static GtkWidget *status_label;
static GtkWidget *log_text;
static GtkTextBuffer *log_buffer;

// Widgets for real-time measurement display
static GtkWidget *sma1_value_label;
static GtkWidget *sma2_value_label;
static GtkWidget *sma3_value_label;
static GtkWidget *sma4_value_label;
static GtkWidget *sma1_status_label;
static GtkWidget *sma2_status_label;
static GtkWidget *sma3_status_label;
static GtkWidget *sma4_status_label;

// Measurement values storage
static char sma_values[4][64] = {"--", "--", "--", "--"};
static int sma_measurement_count[4] = {0, 0, 0, 0};
static gboolean measurement_active = FALSE;
static guint update_timer_id = 0;
static FILE *measurement_pipe = NULL;

void add_log_message(const char *message) {
    gtk_text_buffer_insert_at_cursor(log_buffer, message, -1);
    
    // Auto-scroll to end
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(log_buffer, &end_iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(log_text), &end_iter, 0.0, FALSE, 0.0, 0.0);
}

void update_sma_display(int channel, const char* value) {
    GtkWidget *value_label = NULL;
    GtkWidget *status_label = NULL;
    
    switch(channel) {
        case 1:
            value_label = sma1_value_label;
            status_label = sma1_status_label;
            break;
        case 2:
            value_label = sma2_value_label;
            status_label = sma2_status_label;
            break;
        case 3:
            value_label = sma3_value_label;
            status_label = sma3_status_label;
            break;
        case 4:
            value_label = sma4_value_label;
            status_label = sma4_status_label;
            break;
        default:
            return;
    }
    
    if (value_label) {
        gtk_label_set_text(GTK_LABEL(value_label), value);
        strcpy(sma_values[channel-1], value);
        sma_measurement_count[channel-1]++;
        
        char status_text[64];
        snprintf(status_text, sizeof(status_text), "Измерений: %d", sma_measurement_count[channel-1]);
        gtk_label_set_text(GTK_LABEL(status_label), status_text);
    }
}

gboolean read_measurement_data(gpointer user_data) {
    if (!measurement_active || !measurement_pipe) {
        return TRUE; // Continue timer
    }
    
    char line[256];
    while (fgets(line, sizeof(line), measurement_pipe) != NULL) {
        // Parse measurement output: "*****Measurement channel X: Y ns"
        if (strstr(line, "*****Measurement channel")) {
            int channel;
            int value;
            char sign = '+';
            
            if (sscanf(line, "*****Measurement channel %d: -%u ns", &channel, &value) == 2) {
                sign = '-';
            } else if (sscanf(line, "*****Measurement channel %d: %u ns", &channel, &value) == 2) {
                sign = '+';
            } else {
                continue;
            }
            
            char display_value[64];
            snprintf(display_value, sizeof(display_value), "%c%u ns", sign, value);
            
            update_sma_display(channel, display_value);
            
            // Add to log
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "SMA%d: %s\n", channel, display_value);
            add_log_message(log_msg);
        }
    }
    
    return TRUE; // Continue timer
}

void start_measurement_monitoring() {
    if (measurement_active) {
        return;
    }
    
    measurement_active = TRUE;
    
    // Reset counters
    for (int i = 0; i < 4; i++) {
        sma_measurement_count[i] = 0;
        strcpy(sma_values[i], "--");
    }
    
    // Update display
    update_sma_display(1, "--");
    update_sma_display(2, "--");
    update_sma_display(3, "--");
    update_sma_display(4, "--");
    
    // Start timer for reading data
    update_timer_id = g_timeout_add(100, read_measurement_data, NULL); // Update every 100ms
}

void stop_measurement_monitoring() {
    measurement_active = FALSE;
    
    if (update_timer_id > 0) {
        g_source_remove(update_timer_id);
        update_timer_id = 0;
    }
    
    if (measurement_pipe) {
        pclose(measurement_pipe);
        measurement_pipe = NULL;
    }
    
    gtk_label_set_text(GTK_LABEL(status_label), "Измерение остановлено");
}

void on_start_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    if (measurement_active) {
        add_log_message("Измерение уже активно!\n");
        return;
    }
    
    char command[1024];
    snprintf(command, sizeof(command), "./OpenTimeInstrument -d %s -e -1 2>&1", device_path);
    
    measurement_pipe = popen(command, "r");
    if (!measurement_pipe) {
        add_log_message("Ошибка запуска измерений!\n");
        return;
    }
    
    start_measurement_monitoring();
    
    add_log_message("Запуск измерений...\n");
    gtk_label_set_text(GTK_LABEL(status_label), "Измерение активно");
}

void on_stop_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    stop_measurement_monitoring();
    add_log_message("Измерения остановлены\n");
}

void on_setup_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    add_log_message("Настройка Time Card...\n");
    
    system("echo IN: TS1 >> /sys/class/timecard/ocp0/sma1");
    system("echo IN: TS2 >> /sys/class/timecard/ocp0/sma2");
    system("echo IN: TS3 >> /sys/class/timecard/ocp0/sma3");
    system("echo IN: TS4 >> /sys/class/timecard/ocp0/sma4");
    
    add_log_message("Time Card настроен\n");
}

void on_check_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    char command[1024];
    snprintf(command, sizeof(command), "ls -la %s", device_path);
    system(command);
    
    add_log_message("Проверка устройства завершена\n");
}

void on_window_destroy(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    stop_measurement_monitoring();
    gtk_main_quit();
}

GtkWidget* create_sma_frame(const char* title, GtkWidget** value_label, GtkWidget** status_label) {
    GtkWidget *frame = gtk_frame_new(title);
    GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    *value_label = gtk_label_new("--");
    gtk_label_set_markup(GTK_LABEL(*value_label), "<span font='16' weight='bold'>--</span>");
    gtk_box_pack_start(GTK_BOX(vbox), *value_label, FALSE, FALSE, 0);
    
    *status_label = gtk_label_new("Измерений: 0");
    gtk_label_set_markup(GTK_LABEL(*status_label), "<span font='10'>Измерений: 0</span>");
    gtk_box_pack_start(GTK_BOX(vbox), *status_label, FALSE, FALSE, 0);
    
    return frame;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OpenTimeInstrument GUI - Измерения в реальном времени");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    GtkWidget *main_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);
    
    // Device selection section
    GtkWidget *device_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), device_hbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(device_hbox), gtk_label_new("Устройство:"), FALSE, FALSE, 0);
    device_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(device_entry), "/dev/ptp1");
    gtk_box_pack_start(GTK_BOX(device_hbox), device_entry, TRUE, TRUE, 0);
    
    GtkWidget *check_button = gtk_button_new_with_label("Проверить");
    g_signal_connect(check_button, "clicked", G_CALLBACK(on_check_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(device_hbox), check_button, FALSE, FALSE, 0);
    
    // Control buttons section
    GtkWidget *buttons_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), buttons_hbox, FALSE, FALSE, 0);
    
    GtkWidget *setup_button = gtk_button_new_with_label("Настроить Time Card");
    g_signal_connect(setup_button, "clicked", G_CALLBACK(on_setup_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), setup_button, FALSE, FALSE, 0);
    
    GtkWidget *start_button = gtk_button_new_with_label("Начать измерение");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), start_button, FALSE, FALSE, 0);
    
    GtkWidget *stop_button = gtk_button_new_with_label("Остановить измерение");
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), stop_button, FALSE, FALSE, 0);
    
    // Status label
    status_label = gtk_label_new("Готов к работе");
    gtk_box_pack_start(GTK_BOX(main_vbox), status_label, FALSE, FALSE, 0);
    
    // Real-time measurements section
    GtkWidget *measurements_frame = gtk_frame_new("Измерения в реальном времени");
    gtk_box_pack_start(GTK_BOX(main_vbox), measurements_frame, FALSE, FALSE, 0);
    
    GtkWidget *measurements_hbox = gtk_hbox_new(TRUE, 10);
    gtk_container_add(GTK_CONTAINER(measurements_frame), measurements_hbox);
    
    // SMA measurement displays
    GtkWidget *sma1_frame = create_sma_frame("SMA1 (TS1)", &sma1_value_label, &sma1_status_label);
    gtk_box_pack_start(GTK_BOX(measurements_hbox), sma1_frame, TRUE, TRUE, 0);
    
    GtkWidget *sma2_frame = create_sma_frame("SMA2 (TS2)", &sma2_value_label, &sma2_status_label);
    gtk_box_pack_start(GTK_BOX(measurements_hbox), sma2_frame, TRUE, TRUE, 0);
    
    GtkWidget *sma3_frame = create_sma_frame("SMA3 (TS3)", &sma3_value_label, &sma3_status_label);
    gtk_box_pack_start(GTK_BOX(measurements_hbox), sma3_frame, TRUE, TRUE, 0);
    
    GtkWidget *sma4_frame = create_sma_frame("SMA4 (TS4)", &sma4_value_label, &sma4_status_label);
    gtk_box_pack_start(GTK_BOX(measurements_hbox), sma4_frame, TRUE, TRUE, 0);
    
    // Log section
    GtkWidget *log_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(main_vbox), log_scrolled, TRUE, TRUE, 0);
    
    log_text = gtk_text_view_new();
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scrolled), log_text);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}