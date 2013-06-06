/*
 * Quote Plugin
 *
 * Copyright (C) 2009, Anton Pomozov <pomozoff@gmail.com>
 *
 * Used sources of the "Character counting plugin for Pidgin"
 * Thank you, Dossy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */

#include "internal.h"
#include "version.h"
#include "pidginstock.h"
#include "gtkutils.h"
#include "gtkimhtml.h"
//#include "debug.h"

#define PLUGIN_ID "gtk-quotetext"
#define QUOTE_VERSION "0.9.3"

/* config.h may define PURPLE_PLUGINS; protect the definition here so that we
 * don't get complaints about redefinition when it's not necessary. */
#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif
/* This will prevent compiler errors in some instances and is better explained in the
 * how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

static void quote_button_press(GtkWidget *button, PidginConversation *gtkconv){
	GtkIMHtml *imhtml = GTK_IMHTML(gtkconv->imhtml);
	GtkTextBuffer *buffer = imhtml->text_buffer;
	GtkTextIter start, *pstart = &start, end, *pend = &end;
	gchar *pBuf = "";
	gint count = 0;
	
	if (gtk_text_buffer_get_selection_bounds(buffer, pstart, pend)) {
		gint i = 0;
		GtkTextIter iter, *piter = &iter;
		gchar *quote_text = "> ";
		GtkTextBuffer *tmpbuf = gtk_text_buffer_new(gtk_text_buffer_get_tag_table(buffer));
		
		gtk_text_buffer_get_start_iter(tmpbuf, piter);
		gtk_text_buffer_insert_range(tmpbuf, piter, pstart, pend);

		count = gtk_text_buffer_get_line_count(tmpbuf);
		for (i = 0; i < count; ++i) {
			gtk_text_iter_set_line(piter, i);
			gtk_text_buffer_insert(tmpbuf, piter, quote_text, -1);
		}
		
		gtk_text_buffer_get_bounds (tmpbuf, pstart, pend);
		pBuf = g_strdup_printf("%s\n", gtk_text_buffer_get_text(tmpbuf, pstart, pend, FALSE));
	} else {
		gtk_text_buffer_get_end_iter(buffer, pend);
		gtk_text_iter_set_line_offset(pend, 0);
		
		do {
			start = end;
			if ( gtk_text_iter_forward_visible_word_end(pend) && !gtk_text_iter_equal(pstart, pend) ){
				gtk_text_iter_forward_to_line_end(pend);
				pBuf = gtk_text_buffer_get_text(buffer, pstart, pend, FALSE);

				count = strlen(pBuf);
				if ( count > 0 ) {
					pBuf = g_strdup_printf("> %s\n", pBuf);
					break;
				}
			}
		} while (gtk_text_iter_backward_line(pend));
		
		if( count <= 0 )
			return;
	}
	
 	gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, pBuf, -1);
 	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
	gtk_widget_grab_focus(gtkconv->entry);
	
 	g_free(pBuf);
	
//	purple_debug_error("quote", "line: %s\n", pBuf);
}

static void attach_to_gtkconv(PidginConversation *gtkconv, gpointer null){
	GtkWidget *toolbar = gtkconv->toolbar, *quote_button = NULL, *sep = NULL, *image = NULL, *label = NULL, *bbox = NULL;

	quote_button = g_object_get_data(G_OBJECT(toolbar), PLUGIN_ID "-quote");
	g_return_if_fail(quote_button == NULL);

	sep = gtk_vseparator_new();
	g_object_set_data(G_OBJECT(toolbar), PLUGIN_ID "-sep", sep);
	gtk_box_pack_start(GTK_BOX(toolbar), sep, FALSE, FALSE, 0);
	gtk_widget_show_all(sep);

	quote_button = gtk_toggle_button_new();
	gtk_widget_set_name(quote_button, "convquote_button");
	gtk_button_set_relief(GTK_BUTTON(quote_button), GTK_RELIEF_NONE);
	g_object_set_data(G_OBJECT(toolbar), PLUGIN_ID "-quote", quote_button);
	g_signal_connect_swapped(G_OBJECT(quote_button), "button-press-event", G_CALLBACK(gtk_widget_activate), quote_button);
	g_signal_connect(G_OBJECT(quote_button), "activate", G_CALLBACK(quote_button_press), gtkconv);
	
	bbox = gtk_hbox_new(FALSE, 3);
	g_object_set_data(G_OBJECT(quote_button), PLUGIN_ID "-bbox", bbox);

	image = gtk_image_new_from_stock(GTK_STOCK_EDIT, gtk_icon_size_from_name(PIDGIN_ICON_SIZE_TANGO_EXTRA_SMALL));
	g_object_set_data(G_OBJECT(bbox), PLUGIN_ID "-image", image);
	gtk_box_pack_start(GTK_BOX(bbox), image, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic(_("_Quote"));
	g_object_set_data(G_OBJECT(bbox), PLUGIN_ID "-label", label);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(quote_button), bbox);
	gtk_box_pack_start(GTK_BOX(toolbar), quote_button, FALSE, FALSE, 0);

	gtk_widget_show_all(quote_button);
}
static void conv_created_quote(PurpleConversation *conv, gpointer null){
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

	g_return_if_fail(gtkconv != NULL);

	attach_to_gtkconv(gtkconv, NULL);
}
static void attach_to_pidgin_window(PidginWindow *win, gpointer null){
	g_list_foreach(pidgin_conv_window_get_gtkconvs(win), (GFunc)attach_to_gtkconv, NULL);
}
static void attach_to_all_windows(){
	g_list_foreach(pidgin_conv_windows_get_list(), (GFunc)attach_to_pidgin_window, NULL);
}

static void detach_from_gtkconv(PidginConversation *gtkconv, gpointer null){
	GtkWidget *toolbar = gtkconv->toolbar, *quote_button = NULL, *sep = NULL, *image = NULL, *label = NULL, *bbox = NULL;

	sep = g_object_get_data(G_OBJECT(toolbar), PLUGIN_ID "-sep");
	if (sep){
		gtk_container_remove(GTK_CONTAINER(toolbar), sep);
		g_object_set_data(G_OBJECT(toolbar), PLUGIN_ID "-sep", NULL);
	}
	
	quote_button = g_object_get_data(G_OBJECT(toolbar), PLUGIN_ID "-quote");
	if (quote_button){
		bbox = g_object_get_data(G_OBJECT(quote_button), PLUGIN_ID "-bbox");
		if (bbox){
			label = g_object_get_data(G_OBJECT(bbox), PLUGIN_ID "-label");
			if (label){
				gtk_container_remove(GTK_CONTAINER(bbox), label);
				g_object_set_data(G_OBJECT(bbox), PLUGIN_ID "-label", NULL);
			}
		
			image = g_object_get_data(G_OBJECT(bbox), PLUGIN_ID "-image");
			if (image){
				gtk_container_remove(GTK_CONTAINER(bbox), image);
				g_object_set_data(G_OBJECT(bbox), PLUGIN_ID "-image", NULL);
			}
	
			gtk_container_remove(GTK_CONTAINER(quote_button), bbox);
			g_object_set_data(G_OBJECT(quote_button), PLUGIN_ID "-bbox", NULL);
		}
	
		g_signal_handlers_disconnect_by_func(G_OBJECT(quote_button), (GFunc)quote_button_press, gtkconv);
		gtk_container_remove(GTK_CONTAINER(toolbar), quote_button);
		g_object_set_data(G_OBJECT(toolbar), PLUGIN_ID "-quote", NULL);
	}

	gtk_widget_queue_draw(pidgin_conv_get_window(gtkconv)->window);
}
static void detach_from_pidgin_window(PidginWindow *win, gpointer null){
	g_list_foreach(pidgin_conv_window_get_gtkconvs(win), (GFunc)detach_from_gtkconv, NULL);
}
static void detach_from_all_windows(){
	g_list_foreach(pidgin_conv_windows_get_list(), (GFunc)detach_from_pidgin_window, NULL);
}

static gboolean plugin_load(PurplePlugin * plugin){
	attach_to_all_windows();

	purple_signal_connect(purple_conversations_get_handle(),
			"conversation-created",
			plugin,
			PURPLE_CALLBACK(conv_created_quote),
			NULL);

	return TRUE;
}
static gboolean plugin_unload(PurplePlugin *plugin){
	detach_from_all_windows();
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"core-quote_text",
	"Quote text",
	QUOTE_VERSION,
	"Quotes selected message to entry area",
	"Quotes selected message to entry area",
	"Anton Pomozov <pomozoff@gmail.com>",
	"http://launchpad.net/quote/",
	plugin_load,
	plugin_unload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void init_plugin (PurplePlugin * plugin){
}

PURPLE_INIT_PLUGIN (quote_text, init_plugin, info)

