#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
    int x, y;
    int gate_type;
    gboolean is_input;
    gboolean state;
    int connection_count;
    int connections[10];
} Node;

typedef struct
{
    const char *name;
    int x, y, height;
    int start_node;
    int node_count;
    int input_count;
    int inputs[16];
    int output_count;
    int outputs[16];
} Gate;

typedef struct
{
    const char *name;
    int node_count;
    int input_count;
    int inputs[16];
    int output_count;
    int outputs[16];
    Node nodes[100];
} SavedGate;

typedef struct
{
    GtkWidget *drawing_area;
    int x, y, type;
} AddCallback;

typedef struct
{
    GtkWidget *drawing_area;
    int index;
} DeleteCallback;

int input_count = 1;
int output_count = 1;

int node_count = 0;
Node nodes[132];

int gate_count = 0;
Gate gates[100];

int saved_gate_count = 0;
SavedGate saved_gates[100];

int current_input = -1;
int current_output = -1;
int current_gate_input = -1;
int current_gate_output = -1;
int current_gate = -1;

int drag_offset_x = 0;
int drag_offset_y = 0;

static gboolean on_draw(GtkWidget *drawing_area, cairo_t *cr)
{
    // draw main inputs
    for (int i = 0; i < input_count; i++)
    {
        double r = 1, g = nodes[i].state ? 0 : 1, b = nodes[i].state ? 0 : 1;

        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, nodes[i].x, nodes[i].y, 15, 0, 2 * G_PI);
        cairo_fill(cr);

        // draw connections
        for (int j = 0; j < nodes[i].connection_count; j++)
        {
            cairo_move_to(cr, nodes[i].x, nodes[i].y);
            cairo_line_to(cr, nodes[nodes[i].connections[j]].x, nodes[nodes[i].connections[j]].y);
            cairo_stroke(cr);
        }
    }

    // draw main outputs
    for (int i = 16; i < (16 + output_count); i++)
    {
        double r = 1, g = nodes[i].state ? 0 : 1, b = nodes[i].state ? 0 : 1;

        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, nodes[i].x, nodes[i].y, 15, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    // draw gates
    for (int i = 0; i < gate_count; i++)
    {
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_rectangle(cr, gates[i].x, gates[i].y, 50, gates[i].height);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);

        cairo_move_to(cr, gates[i].x, gates[i].y - (gates[i].input_count > 1 ? 25 : 10));
        cairo_show_text(cr, gates[i].name);

        // draw gate inputs
        for (int j = 0; j < gates[i].input_count; j++)
        {
            int input_index = gates[i].inputs[j];
            double r = 1, g = nodes[input_index].state ? 0 : 1, b = nodes[input_index].state ? 0 : 1;

            cairo_set_source_rgb(cr, r, g, b);
            cairo_arc(cr, nodes[input_index].x, nodes[input_index].y, 15, 0, 2 * G_PI);
            cairo_fill(cr);
        }

        // draw gate outputs
        for (int j = 0; j < gates[i].output_count; j++)
        {
            int output_index = gates[i].outputs[j];
            double r = 1, g = nodes[output_index].state ? 0 : 1, b = nodes[output_index].state ? 0 : 1;

            cairo_set_source_rgb(cr, r, g, b);
            cairo_arc(cr, nodes[output_index].x, nodes[output_index].y, 15, 0, 2 * G_PI);
            cairo_fill(cr);

            // draw connections
            for (int k = 0; k < nodes[output_index].connection_count; k++)
            {
                cairo_move_to(cr, nodes[output_index].x, nodes[output_index].y);
                cairo_line_to(cr, nodes[nodes[output_index].connections[k]].x, nodes[nodes[output_index].connections[k]].y);
                cairo_stroke(cr);
            }
        }
    }

    return FALSE;

    (void)(drawing_area);
}

static void setup_input_nodes(int height)
{
    int x_padding = 30;
    int y_padding = 45;

    int input_height = (input_count - 1) * 3 * 15;
    int input_y_start = (height / 2) - (input_height / 2);

    for (int i = 0; i < input_count; i++)
    {
        nodes[i].x = x_padding;
        nodes[i].y = input_y_start + (i * y_padding);
    }
}

static void setup_output_nodes(int height, int width)
{
    int x_padding = 30;
    int y_padding = 45;

    int output_height = (output_count - 1) * 3 * 15;
    int output_y_start = (height / 2) - (output_height / 2);

    for (int i = 16; i < (16 + output_count); i++)
    {
        nodes[i].x = width - x_padding;
        nodes[i].y = output_y_start + ((i - 16) * y_padding);
    }
}

static void on_size_allocate(GtkWidget *drawing_area, GtkAllocation *allocation)
{
    setup_input_nodes(allocation->height);
    setup_output_nodes(allocation->height, allocation->width);

    (void)(drawing_area);
}

int find_main_input_in_focus(int x, int y)
{
    for (int i = 0; i < input_count; i++)
    {
        double dx = x - nodes[i].x;
        double dy = y - nodes[i].y;
        double distance = sqrt(dx * dx + dy * dy);

        if (distance <= 15)
        {
            return i;
        }
    }

    return -1;
}

int find_main_output_in_focus(int x, int y)
{
    for (int i = 16; i < (16 + output_count); i++)
    {
        double dx = x - nodes[i].x;
        double dy = y - nodes[i].y;
        double distance = sqrt(dx * dx + dy * dy);

        if (distance <= 15)
        {
            return i;
        }
    }

    return -1;
}

int find_gate_input_in_focus(int x, int y)
{
    for (int i = 0; i < gate_count; i++)
    {
        for (int j = 0; j < gates[i].input_count; j++)
        {
            int input_index = gates[i].inputs[j];

            double dx = x - nodes[input_index].x;
            double dy = y - nodes[input_index].y;
            double distance = sqrt(dx * dx + dy * dy);

            if (distance <= 15)
            {
                return input_index;
            }
        }
    }

    return -1;
}

int find_gate_output_in_focus(int x, int y)
{
    for (int i = 0; i < gate_count; i++)
    {
        for (int j = 0; j < gates[i].output_count; j++)
        {
            int output_index = gates[i].outputs[j];

            double dx = x - nodes[output_index].x;
            double dy = y - nodes[output_index].y;
            double distance = sqrt(dx * dx + dy * dy);

            if (distance <= 15)
            {
                return output_index;
            }
        }
    }

    return -1;
}

int find_gate_in_focus(int x, int y)
{
    for (int i = 0; i < gate_count; i++)
    {
        if (x >= gates[i].x && x <= gates[i].x + 50 &&
            y >= gates[i].y && y <= gates[i].y + gates[i].height)
        {
            return i;
        }
    }

    return -1;
}

int is_gate_input_connected(int gate_input_index)
{
    // find connection from main inputs

    for (int i = 0; i < input_count; i++)
    {
        for (int j = 0; j < nodes[i].connection_count; j++)
        {
            if (nodes[i].connections[j] == gate_input_index)
            {
                return i;
            }
        }
    }

    // find connection from gate outputs

    for (int i = 0; i < gate_count; i++)
    {
        for (int j = 0; j < gates[i].output_count; j++)
        {
            int output_index = gates[i].outputs[j];

            for (int k = 0; k < nodes[output_index].connection_count; k++)
            {
                if (nodes[output_index].connections[k] == gate_input_index)
                {
                    return output_index;
                }
            }
        }
    }

    return -1;
}

int is_main_output_connected(int main_output_index)
{
    // find connection from gate outputs

    for (int i = 0; i < gate_count; i++)
    {
        for (int j = 0; j < gates[i].output_count; j++)
        {
            int output_index = gates[i].outputs[j];

            for (int k = 0; k < nodes[output_index].connection_count; k++)
            {
                if (nodes[output_index].connections[k] == main_output_index)
                {
                    return output_index;
                }
            }
        }
    }

    return -1;
}

static void calc_gate(int input_index)
{
    int inputA, inputB, output;

    if (nodes[input_index].gate_type == 0) // NOT gate
    {
        inputA = input_index;
        output = input_index + 1;

        nodes[output].state = !nodes[inputA].state;
    }
    else if (nodes[input_index].gate_type == 1) // AND gate
    {
        if (nodes[input_index + 1].is_input == TRUE)
        {
            inputA = input_index;
            inputB = input_index + 1;
            output = input_index + 2;
        }
        else
        {
            inputA = input_index - 1;
            inputB = input_index;
            output = input_index + 1;
        }

        if (nodes[inputA].state == TRUE && nodes[inputB].state == TRUE)
        {
            nodes[output].state = TRUE;
        }
        else
        {
            nodes[output].state = FALSE;
        }
    }
    else
    {
        // input index is "dummy" node
        output = input_index;
    }

    // update connected nodes' state
    for (int i = 0; i < nodes[output].connection_count; i++)
    {
        int conn_index = nodes[output].connections[i];
        nodes[conn_index].state = nodes[output].state;

        // move to connected node / gate
        /* if (nodes[conn_index].gate_type != -1)
        {
            calc_gate(conn_index);
        } */
        calc_gate(conn_index);
    }
}

void delete_connection(int from, int to)
{
    for (int i = 0; i < nodes[from].connection_count; i++)
    {
        if (nodes[from].connections[i] == to)
        {
            nodes[from].connection_count--;

            // shift connections to the left
            for (int j = i; j < nodes[from].connection_count; j++)
            {
                nodes[from].connections[j] = nodes[from].connections[j + 1];
            }
        }
    }
}

int get_bounding_position(int target, int max, int min)
{
    if (target > max)
    {
        return max;
    }
    else if (target < min)
    {
        return min;
    }
    else
    {
        return target;
    }
}

void add_gate(int type, int x, int y)
{
    if (type == 0)
    {
        // gate type 0 -> NOT gate
        Node not_input = {x, y + 15, 0, TRUE, FALSE, 0, {}};
        Node not_output = {x + 50, y + 15, 0, FALSE, TRUE, 0, {}};
        nodes[32 + node_count] = not_input;
        nodes[33 + node_count] = not_output;

        Gate not_gate = {"NOT", x, y, 30, 32 + node_count, 2, 1, {32 + node_count}, 1, {33 + node_count}};
        gates[gate_count] = not_gate;

        node_count += 2;
        gate_count += 1;
    }
    else if (type == 1)
    {
        // gate type 1 -> AND gate
        Node and_inputA = {x, y, 1, TRUE, FALSE, 0, {}};
        Node and_inputB = {x, y + 45, 1, TRUE, FALSE, 0, {}};
        Node and_output = {x + 50, y + 22, 1, FALSE, FALSE, 0, {}};
        nodes[32 + node_count] = and_inputA;
        nodes[33 + node_count] = and_inputB;
        nodes[34 + node_count] = and_output;

        Gate and_gate = {"AND", x, y, 45, 32 + node_count, 3, 2, {32 + node_count, 33 + node_count}, 1, {34 + node_count}};
        gates[gate_count] = and_gate;

        node_count += 3;
        gate_count += 1;
    }
}

void delete_gate(GtkWidget *menuitem, DeleteCallback *callback)
{
    // delete connections to gate inputs
    for (int i = 0; i < gates[callback->index].input_count; i++)
    {
        int from = is_gate_input_connected(gates[callback->index].inputs[i]);

        if (from > -1)
        {
            delete_connection(from, gates[callback->index].inputs[i]);
        }
    }

    // delete nodes / shift nodes array left
    int gate_start_node = gates[callback->index].start_node;
    int gate_node_count = gates[callback->index].node_count;

    node_count -= gate_node_count;
    for (int i = gate_start_node; i < 32 + node_count; i++)
    {
        nodes[i] = nodes[i + gate_node_count];
    }

    // shift main input connections to the left
    for (int i = 0; i < input_count; i++)
    {
        for (int j = 0; j < nodes[i].connection_count; j++)
        {
            if (nodes[i].connections[j] > gate_start_node)
            {
                nodes[i].connections[j] -= gate_node_count;
            }
        }
    }

    // shift node connections to the left
    for (int i = 32; i < 32 + node_count; i++)
    {
        for (int j = 0; j < nodes[i].connection_count; j++)
        {
            if (nodes[i].connections[j] > gate_start_node)
            {
                nodes[i].connections[j] -= gate_node_count;
            }
        }
    }

    for (int i = 0; i < gate_count; i++)
    {
        // shift gate start_node to the left
        if (gates[i].start_node > gate_start_node)
        {
            gates[i].start_node -= gate_node_count;
        }

        // shift gate inputs to the left
        for (int j = 0; j < gates[i].input_count; j++)
        {
            if (gates[i].inputs[j] > gate_start_node)
            {
                gates[i].inputs[j] -= gate_node_count;
            }
        }
        // shift gate outputs to the left
        for (int j = 0; j < gates[i].output_count; j++)
        {
            if (gates[i].outputs[j] > gate_start_node)
            {
                gates[i].outputs[j] -= gate_node_count;
            }
        }
    }

    // delete gate / shift gates array to the left
    gate_count--;
    for (int i = callback->index; i < gate_count; i++)
    {
        gates[i] = gates[i + 1];
    }

    gtk_widget_queue_draw(callback->drawing_area);

    g_free(callback);

    (void)(menuitem);
}

void on_add_gate(GtkWidget *menuitem, AddCallback *callback)
{
    add_gate(callback->type, callback->x, callback->y);
    gtk_widget_queue_draw(callback->drawing_area);

    g_free(callback);

    (void)(menuitem);
}

void add_saved_gate(GtkWidget *menuitem, AddCallback *callback)
{
    SavedGate saved_gate = saved_gates[callback->type];

    Gate new_gate;
    new_gate.x = 200;
    new_gate.y = 200;
    new_gate.start_node = 32 + node_count;
    new_gate.name = saved_gate.name;
    new_gate.node_count = saved_gate.node_count;

    int node_count_copy = node_count;

    // copy saved nodes
    for (int i = 0; i < saved_gate.node_count; i++)
    {
        nodes[32 + node_count] = saved_gate.nodes[i];

        int conn_count = saved_gate.nodes[i].connection_count;
        for (int j = 0; j < conn_count; j++)
        {
            nodes[32 + node_count].connections[j] += node_count_copy;
        }

        node_count++;
    }

    int _input_count = saved_gate.input_count;
    int _output_count = saved_gate.output_count;

    int max = _input_count > _output_count ? _input_count : _output_count;
    new_gate.height = max > 1 ? 15 * 3 * (max - 1) : 15 * 2;

    // position new gate inputs
    for (int i = 0; i < _input_count; i++)
    {
        int y_offset = _input_count < max || _input_count == 1 ? (i + 1) * (new_gate.height / (_input_count + 1)) : i * 15 * 3;

        int input_index = saved_gate.inputs[i] + node_count_copy;
        new_gate.inputs[i] = input_index;

        nodes[input_index].x = new_gate.x;
        nodes[input_index].y = new_gate.y + y_offset;
    }
    new_gate.input_count = _input_count;

    // position new gate outputs
    for (int i = 0; i < _output_count; i++)
    {
        int y_offset = _output_count < max || _output_count == 1 ? (i + 1) * (new_gate.height / (_output_count + 1)) : i * 15 * 3;

        int output_index = saved_gate.outputs[i] + node_count_copy;
        new_gate.outputs[i] = output_index;

        nodes[output_index].x = new_gate.x + 50;
        nodes[output_index].y = new_gate.y + y_offset;
    }
    new_gate.output_count = _output_count;

    gates[gate_count] = new_gate;
    gate_count++;

    gtk_widget_queue_draw(callback->drawing_area);
    g_free(callback);

    (void)(menuitem);
}

static gboolean on_button_press(GtkWidget *drawing_area, GdkEventButton *event)
{
    if (event->button == GDK_BUTTON_PRIMARY)
    {
        current_input = find_main_input_in_focus(event->x, event->y);

        if (current_input > -1)
        {
            return TRUE;
        }

        current_gate_output = find_gate_output_in_focus(event->x, event->y);

        if (current_gate_output > -1)
        {
            return TRUE;
        }

        current_output = find_main_output_in_focus(event->x, event->y);

        if (current_output > -1)
        {
            return TRUE;
        }

        current_gate_input = find_gate_input_in_focus(event->x, event->y);

        if (current_gate_input > -1)
        {
            return TRUE;
        }

        current_gate = find_gate_in_focus(event->x, event->y);

        if (current_gate > -1)
        {
            drag_offset_x = event->x - gates[current_gate].x;
            drag_offset_y = event->y - gates[current_gate].y;

            return TRUE;
        }
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        GtkWidget *menu;
        menu = gtk_menu_new();

        int gate_in_focus = find_gate_in_focus(event->x, event->y);

        if (gate_in_focus > -1)
        {
            GtkWidget *menuitem;
            menuitem = gtk_menu_item_new_with_label("Delete");

            DeleteCallback *callback = g_malloc(sizeof(DeleteCallback));
            callback->drawing_area = drawing_area;
            callback->index = gate_in_focus;

            g_signal_connect(menuitem, "activate", G_CALLBACK(delete_gate), callback);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        }
        else
        {
            const char *gate_types[] = {"NOT", "AND"};
            for (int i = 0; i < 2; i++)
            {
                AddCallback *callback = g_malloc(sizeof(AddCallback));
                callback->drawing_area = drawing_area;
                callback->x = event->x;
                callback->y = event->y;
                callback->type = i;

                GtkWidget *menuitem;
                menuitem = gtk_menu_item_new_with_label(gate_types[i]);

                g_signal_connect(menuitem, "activate", G_CALLBACK(on_add_gate), callback);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
            }

            for (int i = 0; i < saved_gate_count; i++)
            {
                AddCallback *callback = g_malloc(sizeof(AddCallback));
                callback->drawing_area = drawing_area;
                callback->x = event->x;
                callback->y = event->y;
                callback->type = i;

                GtkWidget *menuitem;
                menuitem = gtk_menu_item_new_with_label(saved_gates[i].name);

                g_signal_connect(menuitem, "activate", G_CALLBACK(add_saved_gate), callback);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
            }
        }

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
    }

    return FALSE;

    (void)(drawing_area);
}

static gboolean on_button_release(GtkWidget *drawing_area, GdkEventButton *event)
{
    if (event->button == GDK_BUTTON_PRIMARY)
    {
        if (current_input > -1)
        {
            int target_input = find_main_input_in_focus(event->x, event->y);

            // from: main input | to: main input
            if (target_input == current_input)
            {
                nodes[target_input].state = !nodes[target_input].state;

                // update connected nodes' state
                for (int i = 0; i < nodes[target_input].connection_count; i++)
                {
                    int conn_index = nodes[target_input].connections[i];
                    nodes[conn_index].state = nodes[target_input].state;
                    calc_gate(conn_index);
                }

                gtk_widget_queue_draw(drawing_area);
            }
            else
            {
                int target_gate_input = find_gate_input_in_focus(event->x, event->y);

                // from: main input | to: gate input
                if (target_gate_input > -1)
                {
                    if (is_gate_input_connected(target_gate_input) == -1)
                    {
                        int conn_count = nodes[current_input].connection_count;
                        nodes[current_input].connections[conn_count] = target_gate_input;
                        nodes[current_input].connection_count++;

                        nodes[target_gate_input].state = nodes[current_input].state;
                        calc_gate(target_gate_input);

                        gtk_widget_queue_draw(drawing_area);
                    }
                }
            }
        }
        else if (current_gate_output > 1)
        {
            int target_gate_input = find_gate_input_in_focus(event->x, event->y);

            // from: gate output | to: gate_input
            if (target_gate_input > -1)
            {
                if (is_gate_input_connected(target_gate_input) == -1)
                {
                    int conn_count = nodes[current_gate_output].connection_count;
                    nodes[current_gate_output].connections[conn_count] = target_gate_input;
                    nodes[current_gate_output].connection_count++;

                    nodes[target_gate_input].state = nodes[current_gate_output].state;
                    calc_gate(target_gate_input);

                    gtk_widget_queue_draw(drawing_area);
                }
            }
            else
            {
                int target_output = find_main_output_in_focus(event->x, event->y);

                // from: gate output | to: main output
                if (target_output > -1)
                {

                    if (is_main_output_connected(target_output) == -1)
                    {
                        int conn_count = nodes[current_gate_output].connection_count;
                        nodes[current_gate_output].connections[conn_count] = target_output;
                        nodes[current_gate_output].connection_count++;

                        nodes[target_output].state = nodes[current_gate_output].state;

                        gtk_widget_queue_draw(drawing_area);
                    }
                }
            }
        }
        else if (current_gate_input > -1)
        {
            int target_gate_input = find_gate_input_in_focus(event->x, event->y);

            // disregard simple click on node
            if (current_gate_input != target_gate_input)
            {
                int from = is_gate_input_connected(current_gate_input);

                if (from > -1)
                {
                    delete_connection(from, current_gate_input);
                    gtk_widget_queue_draw(drawing_area);
                }
            }
        }
        else if (current_output > -1)
        {
            int target_main_output = find_main_output_in_focus(event->x, event->y);

            // disregard simple click on node
            if (current_output != target_main_output)
            {
                int from = is_main_output_connected(current_output);

                if (from > -1)
                {
                    delete_connection(from, current_output);
                    gtk_widget_queue_draw(drawing_area);
                }
            }
        }

        current_input = -1;
        current_output = -1;
        current_gate_input = -1;
        current_gate_output = -1;
        current_gate = -1;
        return TRUE;
    }

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *drawing_area, GdkEventMotion *event)
{
    if (current_gate > -1)
    {
        int _input_count = gates[current_gate].input_count;
        int _output_count = gates[current_gate].output_count;
        int GATE_HEIGHT = gates[current_gate].height;

        int max = _input_count > _output_count ? _input_count : _output_count;

        int widget_width = gtk_widget_get_allocated_width(drawing_area);
        int widget_height = gtk_widget_get_allocated_height(drawing_area);

        gates[current_gate].x = get_bounding_position(event->x - drag_offset_x, widget_width - 50, 0);
        gates[current_gate].y = get_bounding_position(event->y - drag_offset_y, widget_height - GATE_HEIGHT, 0);

        for (int i = 0; i < _input_count; i++)
        {
            int y_offset = _input_count < max || _input_count == 1 ? (i + 1) * (GATE_HEIGHT / (_input_count + 1)) : i * 15 * 3;
            nodes[gates[current_gate].inputs[i]].x = get_bounding_position(event->x - drag_offset_x, widget_width - 50, 0);
            nodes[gates[current_gate].inputs[i]].y = get_bounding_position(event->y - drag_offset_y + y_offset, widget_height - GATE_HEIGHT + y_offset, y_offset);
        }

        for (int i = 0; i < _output_count; i++)
        {
            int y_offset = _output_count < max || _output_count == 1 ? (i + 1) * (GATE_HEIGHT / (_output_count + 1)) : i * 15 * 3;
            nodes[gates[current_gate].outputs[i]].x = get_bounding_position(event->x - drag_offset_x + 50, widget_width, 50);
            nodes[gates[current_gate].outputs[i]].y = get_bounding_position(event->y - drag_offset_y + y_offset, widget_height - GATE_HEIGHT + y_offset, y_offset);
        }

        gtk_widget_queue_draw(drawing_area);

        return TRUE;
    }

    return FALSE;
}

static void on_input_spin_button_value_changed(GtkSpinButton *spin_button, GtkWidget *drawing_area)
{
    int new_input_count = gtk_spin_button_get_value_as_int(spin_button);

    if (new_input_count < input_count)
    {
        nodes[new_input_count] = (Node){0};
    }
    input_count = new_input_count;

    int height = gtk_widget_get_allocated_height(drawing_area);

    setup_input_nodes(height);
    gtk_widget_queue_draw(drawing_area);
}

static void on_output_spin_button_value_changed(GtkSpinButton *spin_button, GtkWidget *drawing_area)
{
    int new_output_count = gtk_spin_button_get_value_as_int(spin_button);

    if (new_output_count < output_count)
    {
        nodes[16 + new_output_count] = (Node){0};
    }
    output_count = new_output_count;

    int height = gtk_widget_get_allocated_height(drawing_area);
    int width = gtk_widget_get_allocated_width(drawing_area);

    setup_output_nodes(height, width);
    gtk_widget_queue_draw(drawing_area);
}

void save_gate(const char *text)
{
    SavedGate new_saved_gate;
    new_saved_gate.name = text;
    new_saved_gate.node_count = 0;

    // copy nodes

    for (int i = 0; i < node_count; i++)
    {
        new_saved_gate.nodes[i] = nodes[i + 32];
        new_saved_gate.node_count++;
    }
    node_count = 0;

    // make new input nodes
    for (int i = 0; i < input_count; i++)
    {
        Node new_input;
        new_input.x = 0;
        new_input.y = 0;
        new_input.gate_type = -1;
        new_input.is_input = TRUE;
        new_input.state = FALSE;
        new_input.connection_count = nodes[i].connection_count;

        for (int j = 0; j < nodes[i].connection_count; j++)
        {
            new_input.connections[j] = nodes[i].connections[j];
        }

        new_saved_gate.nodes[new_saved_gate.node_count] = new_input;
        new_saved_gate.inputs[i] = 32 + new_saved_gate.node_count;
        new_saved_gate.node_count++;

        nodes[i].state = FALSE;
        nodes[i].connection_count = 0;
    }
    new_saved_gate.input_count = input_count;

    // make new output nodes
    for (int i = 0; i < output_count; i++)
    {
        Node new_output;
        new_output.x = 0;
        new_output.y = 0;
        new_output.gate_type = -1;
        new_output.is_input = FALSE;
        new_output.state = FALSE;
        new_output.connection_count = 0;

        int connected_node = is_main_output_connected(16 + i) - 32;

        if (connected_node > -1)
        {
            new_output.state = new_saved_gate.nodes[connected_node].state;

            // find connection to main output
            for (int j = 0; j < new_saved_gate.nodes[connected_node].connection_count; j++)
            {
                if (new_saved_gate.nodes[connected_node].connections[j] == 16 + i)
                {
                    new_saved_gate.nodes[connected_node].connections[j] = 32 + new_saved_gate.node_count;
                    break;
                }
            }
        }

        new_saved_gate.nodes[new_saved_gate.node_count] = new_output;
        new_saved_gate.outputs[i] = 32 + new_saved_gate.node_count;
        new_saved_gate.node_count++;

        nodes[i + 16].state = FALSE;
        nodes[i + 16].connection_count = 0;
    }
    new_saved_gate.output_count = output_count;

    for (int i = 0; i < gate_count; i++)
    {
        gates[i] = (Gate){0};
    }
    gate_count = 0;

    saved_gates[saved_gate_count] = new_saved_gate;
    saved_gate_count++;

    (void)(text);
}

static void on_save(GtkButton *button, GtkWidget *drawing_area)
{
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *content_area;
    const char *text;

    dialog = gtk_dialog_new_with_buttons("Save", NULL, GTK_DIALOG_MODAL, "OK", GTK_RESPONSE_ACCEPT, NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();

    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        text = gtk_entry_get_text(GTK_ENTRY(entry));

        save_gate(g_strdup(text));
        gtk_widget_queue_draw(drawing_area);
    }

    gtk_widget_destroy(dialog);

    (void)(button);
}

int main(int argc, char **argv)
{
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *save_button;
    GtkSpinButton *input_spin_button;
    GtkSpinButton *output_spin_button;

    GtkBuilder *builder = NULL;

    add_gate(0, 200, 200);
    add_gate(1, 400, 200);

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();

    if (gtk_builder_add_from_file(builder, "ui.glade", NULL) == 0)
    {
        printf("gtk_builder_add_from_file FAILED\n");
        return 0;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "MyWindow"));
    drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "MyDrawingArea"));
    save_button = GTK_WIDGET(gtk_builder_get_object(builder, "SaveButton"));
    input_spin_button = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "InputSpinButton"));
    output_spin_button = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "OutputSpinButton"));

    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "size-allocate", G_CALLBACK(on_size_allocate), NULL);

    g_signal_connect(G_OBJECT(input_spin_button), "value-changed", G_CALLBACK(on_input_spin_button_value_changed), drawing_area);
    g_signal_connect(G_OBJECT(output_spin_button), "value-changed", G_CALLBACK(on_output_spin_button_value_changed), drawing_area);

    g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(on_save), drawing_area);

    gtk_widget_set_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

    gtk_builder_connect_signals(builder, NULL);
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

void exit_window()
{
    gtk_main_quit();
}