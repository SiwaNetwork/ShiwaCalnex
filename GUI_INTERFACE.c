#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GtkWidget *device_entry;
static GtkWidget *status_label;
static GtkWidget *log_text;
static GtkTextBuffer *log_buffer;

void add_log_message(const char *message) {
    gtk_text_buffer_insert_at_cursor(log_buffer, message, -1);
}

void on_start_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    char command[1024];  // Увеличен размер буфера
    snprintf(command, sizeof(command), "./OpenTimeInstrument -d %s -e -1 &", device_path);
    
    add_log_message("Запуск измерений...\n");
    gtk_label_set_text(GTK_LABEL(status_label), "Измерение активно");
    
    system(command);
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
    
    char command[1024];  // Увеличен размер буфера
    snprintf(command, sizeof(command), "ls -la %s", device_path);
    system(command);
    
    add_log_message("Проверка устройства завершена\n");
}

void on_window_destroy(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OpenTimeInstrument GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    GtkWidget *vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    GtkWidget *device_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), device_hbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(device_hbox), gtk_label_new("Устройство:"), FALSE, FALSE, 0);
    device_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(device_entry), "/dev/ptp1");
    gtk_box_pack_start(GTK_BOX(device_hbox), device_entry, TRUE, TRUE, 0);
    
    GtkWidget *check_button = gtk_button_new_with_label("Проверить");
    g_signal_connect(check_button, "clicked", G_CALLBACK(on_check_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(device_hbox), check_button, FALSE, FALSE, 0);
    
    GtkWidget *buttons_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), buttons_hbox, FALSE, FALSE, 0);
    
    GtkWidget *setup_button = gtk_button_new_with_label("Настроить Time Card");
    g_signal_connect(setup_button, "clicked", G_CALLBACK(on_setup_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), setup_button, FALSE, FALSE, 0);
    
    GtkWidget *start_button = gtk_button_new_with_label("Начать измерение");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), start_button, FALSE, FALSE, 0);
    
    status_label = gtk_label_new("Готов к работе");
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
    
    GtkWidget *log_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), log_scrolled, TRUE, TRUE, 0);
    
    log_text = gtk_text_view_new();
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scrolled), log_text);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}