#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include <math.h>
#include <time.h>

#define MAX_DATA_POINTS 1000
#define NUM_CHANNELS 4

// Data structures for measurements
typedef struct {
    double values[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    int count;
    int head;
    double min_val;
    double max_val;
    double avg_val;
    int total_measurements;
} ChannelData;

// Global variables
static GtkWidget *device_entry;
static GtkWidget *status_label;
static GtkWidget *log_text;
static GtkTextBuffer *log_buffer;

// Widgets for real-time measurement display
static GtkWidget *sma_value_labels[NUM_CHANNELS];
static GtkWidget *sma_status_labels[NUM_CHANNELS];
static GtkWidget *graph_widgets[NUM_CHANNELS];
static GtkWidget *combined_graph_widget;

// Data storage
static ChannelData channel_data[NUM_CHANNELS];
static gboolean measurement_active = FALSE;
static guint update_timer_id = 0;
static FILE *measurement_pipe = NULL;

// Graph colors for each channel
static double channel_colors[NUM_CHANNELS][3] = {
    {0.0, 0.5, 1.0},  // Blue
    {0.0, 0.8, 0.0},  // Green
    {1.0, 0.6, 0.0},  // Orange
    {1.0, 0.0, 0.0}   // Red
};

void init_channel_data() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channel_data[i].count = 0;
        channel_data[i].head = 0;
        channel_data[i].min_val = 0;
        channel_data[i].max_val = 0;
        channel_data[i].avg_val = 0;
        channel_data[i].total_measurements = 0;
        
        for (int j = 0; j < MAX_DATA_POINTS; j++) {
            channel_data[i].values[j] = 0.0;
            channel_data[i].timestamps[j] = 0;
        }
    }
}

void add_measurement(int channel, double value) {
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    
    ChannelData *data = &channel_data[channel];
    
    // Add to circular buffer
    data->values[data->head] = value;
    data->timestamps[data->head] = time(NULL);
    data->head = (data->head + 1) % MAX_DATA_POINTS;
    
    if (data->count < MAX_DATA_POINTS) {
        data->count++;
    }
    
    data->total_measurements++;
    
    // Update statistics
    if (data->total_measurements == 1) {
        data->min_val = data->max_val = data->avg_val = value;
    } else {
        if (value < data->min_val) data->min_val = value;
        if (value > data->max_val) data->max_val = value;
        
        // Calculate running average
        double sum = 0;
        int start = (data->count < MAX_DATA_POINTS) ? 0 : data->head;
        for (int i = 0; i < data->count; i++) {
            int idx = (start + i) % MAX_DATA_POINTS;
            sum += data->values[idx];
        }
        data->avg_val = sum / data->count;
    }
}

void add_log_message(const char *message) {
    gtk_text_buffer_insert_at_cursor(log_buffer, message, -1);
    
    // Auto-scroll to end
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(log_buffer, &end_iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(log_text), &end_iter, 0.0, FALSE, 0.0, 0.0);
}

void update_sma_display(int channel) {
    if (channel < 0 || channel >= NUM_CHANNELS) return;
    
    ChannelData *data = &channel_data[channel];
    
    if (data->count > 0) {
        int latest_idx = (data->head - 1 + MAX_DATA_POINTS) % MAX_DATA_POINTS;
        double latest_value = data->values[latest_idx];
        
        char value_text[64];
        snprintf(value_text, sizeof(value_text), "%.0f нс", latest_value);
        gtk_label_set_markup(GTK_LABEL(sma_value_labels[channel]), 
                           g_strdup_printf("<span font='16' weight='bold'>%s</span>", value_text));
        
        char status_text[128];
        snprintf(status_text, sizeof(status_text), 
                "Измерений: %d\nМин: %.0f нс\nМакс: %.0f нс\nСр: %.1f нс", 
                data->total_measurements, data->min_val, data->max_val, data->avg_val);
        gtk_label_set_markup(GTK_LABEL(sma_status_labels[channel]), 
                           g_strdup_printf("<span font='10'>%s</span>", status_text));
    }
}

gboolean read_measurement_data(gpointer user_data) {
    if (!measurement_active || !measurement_pipe) {
        return TRUE;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), measurement_pipe) != NULL) {
        // Parse measurement output: "*****Measurement channel X: Y ns"
        if (strstr(line, "*****Measurement channel")) {
            int channel;
            int value;
            
            if (sscanf(line, "*****Measurement channel %d: -%d ns", &channel, &value) == 2) {
                add_measurement(channel - 1, -value);
                update_sma_display(channel - 1);
            } else if (sscanf(line, "*****Measurement channel %d: %d ns", &channel, &value) == 2) {
                add_measurement(channel - 1, value);
                update_sma_display(channel - 1);
            }
            
            // Trigger graph redraws
            for (int i = 0; i < NUM_CHANNELS; i++) {
                gtk_widget_queue_draw(graph_widgets[i]);
            }
            gtk_widget_queue_draw(combined_graph_widget);
        }
        
        // Add to log
        add_log_message(line);
    }
    
    return TRUE; // Continue calling this function
}

// Graph drawing function
gboolean on_graph_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int channel = GPOINTER_TO_INT(user_data);
    gint width, height;
    
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    
    // Clear background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Draw border
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_stroke(cr);
    
    if (channel < 0 || channel >= NUM_CHANNELS) return FALSE;
    
    ChannelData *data = &channel_data[channel];
    
    if (data->count < 2) return FALSE;
    
    // Calculate scale
    double margin = 20;
    double plot_width = width - 2 * margin;
    double plot_height = height - 2 * margin;
    
    double y_range = data->max_val - data->min_val;
    if (y_range < 10) y_range = 10; // Minimum range for visualization
    
    // Draw grid
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_set_line_width(cr, 0.5);
    
    for (int i = 1; i < 10; i++) {
        double y = margin + (plot_height * i) / 10;
        cairo_move_to(cr, margin, y);
        cairo_line_to(cr, width - margin, y);
        cairo_stroke(cr);
    }
    
    // Draw data
    cairo_set_source_rgb(cr, channel_colors[channel][0], 
                             channel_colors[channel][1], 
                             channel_colors[channel][2]);
    cairo_set_line_width(cr, 2.0);
    
    int start = (data->count < MAX_DATA_POINTS) ? 0 : data->head;
    gboolean first_point = TRUE;
    
    for (int i = 0; i < data->count; i++) {
        int idx = (start + i) % MAX_DATA_POINTS;
        
        double x = margin + (plot_width * i) / (data->count - 1);
        double y = margin + plot_height - ((data->values[idx] - data->min_val) / y_range) * plot_height;
        
        if (first_point) {
            cairo_move_to(cr, x, y);
            first_point = FALSE;
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    
    cairo_stroke(cr);
    
    // Draw title and current value
    char title[128];
    if (data->count > 0) {
        int latest_idx = (data->head - 1 + MAX_DATA_POINTS) % MAX_DATA_POINTS;
        snprintf(title, sizeof(title), "Канал %d: %.0f нс", channel + 1, data->values[latest_idx]);
    } else {
        snprintf(title, sizeof(title), "Канал %d: -- нс", channel + 1);
    }
    
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 10, 15);
    cairo_show_text(cr, title);
    
    return FALSE;
}

// Combined graph drawing function
gboolean on_combined_graph_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    gint width, height;
    
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    
    // Clear background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Draw border
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_stroke(cr);
    
    // Find global min/max
    double global_min = 0, global_max = 0;
    int max_count = 0;
    gboolean has_data = FALSE;
    
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        if (channel_data[ch].count > 0) {
            if (!has_data) {
                global_min = channel_data[ch].min_val;
                global_max = channel_data[ch].max_val;
                has_data = TRUE;
            } else {
                if (channel_data[ch].min_val < global_min) global_min = channel_data[ch].min_val;
                if (channel_data[ch].max_val > global_max) global_max = channel_data[ch].max_val;
            }
            if (channel_data[ch].count > max_count) max_count = channel_data[ch].count;
        }
    }
    
    if (!has_data || max_count < 2) return FALSE;
    
    double margin = 40;
    double plot_width = width - 2 * margin;
    double plot_height = height - 2 * margin;
    
    double y_range = global_max - global_min;
    if (y_range < 10) y_range = 10;
    
    // Draw grid
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_set_line_width(cr, 0.5);
    
    for (int i = 1; i < 10; i++) {
        double y = margin + (plot_height * i) / 10;
        cairo_move_to(cr, margin, y);
        cairo_line_to(cr, width - margin, y);
        cairo_stroke(cr);
    }
    
    // Draw each channel
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        ChannelData *data = &channel_data[ch];
        
        if (data->count < 2) continue;
        
        cairo_set_source_rgb(cr, channel_colors[ch][0], 
                                 channel_colors[ch][1], 
                                 channel_colors[ch][2]);
        cairo_set_line_width(cr, 2.0);
        
        int start = (data->count < MAX_DATA_POINTS) ? 0 : data->head;
        gboolean first_point = TRUE;
        
        for (int i = 0; i < data->count; i++) {
            int idx = (start + i) % MAX_DATA_POINTS;
            
            double x = margin + (plot_width * i) / (max_count - 1);
            double y = margin + plot_height - ((data->values[idx] - global_min) / y_range) * plot_height;
            
            if (first_point) {
                cairo_move_to(cr, x, y);
                first_point = FALSE;
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        
        cairo_stroke(cr);
    }
    
    // Draw legend
    cairo_set_font_size(cr, 10);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        cairo_set_source_rgb(cr, channel_colors[ch][0], 
                                 channel_colors[ch][1], 
                                 channel_colors[ch][2]);
        
        double legend_x = width - 100;
        double legend_y = 20 + ch * 15;
        
        cairo_rectangle(cr, legend_x, legend_y - 8, 10, 10);
        cairo_fill(cr);
        
        char legend_text[32];
        snprintf(legend_text, sizeof(legend_text), "Канал %d", ch + 1);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_move_to(cr, legend_x + 15, legend_y);
        cairo_show_text(cr, legend_text);
    }
    
    // Draw title
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, 10, 20);
    cairo_show_text(cr, "Все каналы - TIE измерения");
    
    return FALSE;
}

void start_measurement_monitoring() {
    if (measurement_active) {
        return;
    }
    
    measurement_active = TRUE;
    
    // Reset data
    init_channel_data();
    for (int i = 0; i < NUM_CHANNELS; i++) {
        gtk_label_set_markup(GTK_LABEL(sma_value_labels[i]), "<span font='16' weight='bold'>--</span>");
        gtk_label_set_markup(GTK_LABEL(sma_status_labels[i]), "<span font='10'>Измерений: 0</span>");
    }
    
    update_timer_id = g_timeout_add(100, read_measurement_data, NULL);
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
}

void on_start_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    if (measurement_active) {
        return;
    }
    
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    char command[512];
    
    snprintf(command, sizeof(command), "./OpenTimeInstrument -d %s -e 1,2,3,4 2>&1", device_path);
    
    measurement_pipe = popen(command, "r");
    if (!measurement_pipe) {
        gtk_label_set_text(GTK_LABEL(status_label), "Ошибка: Не удалось запустить измерения");
        return;
    }
    
    start_measurement_monitoring();
    gtk_label_set_text(GTK_LABEL(status_label), "Измерения запущены");
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Запуск измерений на устройстве: %s\n", device_path);
    add_log_message(log_msg);
}

void on_stop_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    stop_measurement_monitoring();
    gtk_label_set_text(GTK_LABEL(status_label), "Измерения остановлены");
    add_log_message("Измерения остановлены\n");
}

void on_clear_data_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    init_channel_data();
    
    for (int i = 0; i < NUM_CHANNELS; i++) {
        gtk_label_set_markup(GTK_LABEL(sma_value_labels[i]), "<span font='16' weight='bold'>--</span>");
        gtk_label_set_markup(GTK_LABEL(sma_status_labels[i]), "<span font='10'>Измерений: 0</span>");
        gtk_widget_queue_draw(graph_widgets[i]);
    }
    gtk_widget_queue_draw(combined_graph_widget);
    
    add_log_message("Данные очищены\n");
}

void on_export_data_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    char filename[256];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    strftime(filename, sizeof(filename), "tie_measurements_%Y%m%d_%H%M%S.csv", tm_info);
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Ошибка создания файла: %s\n", filename);
        add_log_message(msg);
        return;
    }
    
    // Write header
    fprintf(file, "Timestamp,Channel,Value_ns\n");
    
    // Export data from all channels
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        ChannelData *data = &channel_data[ch];
        int start = (data->count < MAX_DATA_POINTS) ? 0 : data->head;
        
        for (int i = 0; i < data->count; i++) {
            int idx = (start + i) % MAX_DATA_POINTS;
            fprintf(file, "%ld,%d,%.0f\n", data->timestamps[idx], ch + 1, data->values[idx]);
        }
    }
    
    fclose(file);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Данные экспортированы в файл: %s\n", filename);
    add_log_message(msg);
}

void on_check_clicked(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    const char *device_path = gtk_entry_get_text(GTK_ENTRY(device_entry));
    
    if (access(device_path, F_OK) == 0) {
        if (access(device_path, R_OK | W_OK) == 0) {
            gtk_label_set_text(GTK_LABEL(status_label), "Устройство доступно");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Устройство найдено, но нет прав доступа");
        }
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Устройство не найдено");
    }
}

void on_window_destroy(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    stop_measurement_monitoring();
    gtk_main_quit();
}

GtkWidget* create_sma_frame(const char* title, int channel) {
    GtkWidget *frame = gtk_frame_new(title);
    GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    sma_value_labels[channel] = gtk_label_new("--");
    gtk_label_set_markup(GTK_LABEL(sma_value_labels[channel]), "<span font='16' weight='bold'>--</span>");
    gtk_box_pack_start(GTK_BOX(vbox), sma_value_labels[channel], FALSE, FALSE, 0);
    
    sma_status_labels[channel] = gtk_label_new("Измерений: 0");
    gtk_label_set_markup(GTK_LABEL(sma_status_labels[channel]), "<span font='10'>Измерений: 0</span>");
    gtk_box_pack_start(GTK_BOX(vbox), sma_status_labels[channel], FALSE, FALSE, 0);
    
    return frame;
}

GtkWidget* create_graph_widget(int channel) {
    GtkWidget *frame = gtk_frame_new(NULL);
    
    char title[64];
    snprintf(title, sizeof(title), "График - Канал %d", channel + 1);
    gtk_frame_set_label(GTK_FRAME(frame), title);
    
    graph_widgets[channel] = gtk_drawing_area_new();
    gtk_widget_set_size_request(graph_widgets[channel], 300, 200);
    
    g_signal_connect(G_OBJECT(graph_widgets[channel]), "draw", 
                     G_CALLBACK(on_graph_draw), GINT_TO_POINTER(channel));
    
    gtk_container_add(GTK_CONTAINER(frame), graph_widgets[channel]);
    
    return frame;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Initialize data
    init_channel_data();
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OpenTimeInstrument GUI Enhanced - Измерения с графиками");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
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
    
    GtkWidget *start_button = gtk_button_new_with_label("Начать измерение");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), start_button, FALSE, FALSE, 0);
    
    GtkWidget *stop_button = gtk_button_new_with_label("Остановить измерение");
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), stop_button, FALSE, FALSE, 0);
    
    GtkWidget *clear_button = gtk_button_new_with_label("Очистить данные");
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_data_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), clear_button, FALSE, FALSE, 0);
    
    GtkWidget *export_button = gtk_button_new_with_label("Экспорт данных");
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_data_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), export_button, FALSE, FALSE, 0);
    
    // Status label
    status_label = gtk_label_new("Готов к работе");
    gtk_box_pack_start(GTK_BOX(main_vbox), status_label, FALSE, FALSE, 0);
    
    // Real-time measurements section
    GtkWidget *measurements_frame = gtk_frame_new("Измерения в реальном времени");
    gtk_box_pack_start(GTK_BOX(main_vbox), measurements_frame, FALSE, FALSE, 0);
    
    GtkWidget *measurements_hbox = gtk_hbox_new(TRUE, 10);
    gtk_container_add(GTK_CONTAINER(measurements_frame), measurements_hbox);
    
    // SMA measurement displays
    const char *sma_titles[] = {"SMA1 (TS1)", "SMA2 (TS2)", "SMA3 (TS3)", "SMA4 (TS4)"};
    for (int i = 0; i < NUM_CHANNELS; i++) {
        GtkWidget *sma_frame = create_sma_frame(sma_titles[i], i);
        gtk_box_pack_start(GTK_BOX(measurements_hbox), sma_frame, TRUE, TRUE, 0);
    }
    
    // Combined graph section
    GtkWidget *combined_frame = gtk_frame_new("Сводный график всех каналов");
    gtk_box_pack_start(GTK_BOX(main_vbox), combined_frame, TRUE, TRUE, 0);
    
    combined_graph_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(combined_graph_widget, -1, 300);
    g_signal_connect(G_OBJECT(combined_graph_widget), "draw", 
                     G_CALLBACK(on_combined_graph_draw), NULL);
    gtk_container_add(GTK_CONTAINER(combined_frame), combined_graph_widget);
    
    // Individual graphs section
    GtkWidget *graphs_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), graphs_notebook, TRUE, TRUE, 0);
    
    for (int i = 0; i < NUM_CHANNELS; i++) {
        GtkWidget *graph_frame = create_graph_widget(i);
        char tab_title[32];
        snprintf(tab_title, sizeof(tab_title), "Канал %d", i + 1);
        gtk_notebook_append_page(GTK_NOTEBOOK(graphs_notebook), graph_frame, gtk_label_new(tab_title));
    }
    
    // Log section
    GtkWidget *log_frame = gtk_frame_new("Журнал событий");
    gtk_box_pack_start(GTK_BOX(main_vbox), log_frame, TRUE, TRUE, 0);
    
    GtkWidget *log_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(log_scrolled, -1, 150);
    gtk_container_add(GTK_CONTAINER(log_frame), log_scrolled);
    
    log_text = gtk_text_view_new();
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text), FALSE);
    gtk_container_add(GTK_CONTAINER(log_scrolled), log_text);
    
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}