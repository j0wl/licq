/*
 * Licq GTK GUI Plugin
 *
 * Copyright (C) 2000, Jon Keating <jon@licq.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "licq_gtk.h"

#include "licq_icqd.h"
#include "licq_user.h"

#include <gtk/gtk.h>

void list_start_convo(GtkWidget *widget, ICQUser *user)
{
	struct conversation *c = 0;

	c = convo_find(user->Uin());

	if(c != 0)
		gdk_window_raise(c->window->window);

	else
	{
		if(user->NewMessages() > 0)
			c = convo_new(user, TRUE);
		else
			c = convo_new(user, FALSE);
	}
}

void list_send_url(GtkWidget *widget, ICQUser *user)
{
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *h_box;
	GtkWidget *close;
	GtkWidget *statusbar;
	const gchar *buffer = "Enter data and send a url";

	struct send_url *url = g_new0(struct send_url, 1);
	url->etag = g_new0(struct e_tag_data, 1);

	url->user = user;

	/* Create the hbox for use late on */
	h_box = gtk_hbox_new(FALSE, 3);

	/* Make the title */
	const gchar *title = g_strdup_printf("URL to %s", user->GetAlias());

	/* Create the window */
	url->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(url->window), title);
	gtk_window_set_position(GTK_WINDOW(url->window), GTK_WIN_POS_CENTER);

	/* The delete_event signal for the window */
	gtk_signal_connect(GTK_OBJECT(url->window), "delete_event",
			   GTK_SIGNAL_FUNC(destroy_dialog), url->window);

	/* Create the table */
	table = gtk_table_new(6, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(url->window), table);

	/* Make the url label and entry */
	label = gtk_label_new("URL:");
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL, GTK_FILL, 3, 3);

	url->entry_u = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), url->entry_u, 1, 2, 0, 1,
			 GtkAttachOptions(GTK_FILL | GTK_EXPAND),
			 GTK_FILL, 3, 3);

	/* Make the description label and entry */
	label = gtk_label_new("Description:");
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL, GTK_FILL, 3, 3);

	url->entry_d = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), url->entry_d, 1, 2, 1, 2,
			 GtkAttachOptions(GTK_FILL | GTK_EXPAND),
			 GTK_FILL, 3, 3);

	/* Make send through server */
	url->send_server = gtk_check_button_new_with_label("Server");
	gtk_table_attach(GTK_TABLE(table), url->send_server, 0, 1, 2, 3,
			 GTK_FILL, GTK_FILL, 3, 3);

	/* Normal, Urgent, or to Contact List */
	url->send_normal = gtk_radio_button_new_with_label(0, "Normal");
	gtk_box_pack_start(GTK_BOX(h_box), url->send_normal, FALSE, FALSE, 0);

	url->send_urgent = gtk_radio_button_new_with_label_from_widget(
				GTK_RADIO_BUTTON(url->send_normal),
				"Urgent");
	gtk_box_pack_start(GTK_BOX(h_box), url->send_urgent, FALSE, FALSE, 0);

	url->send_list = gtk_radio_button_new_with_label_from_widget(
				GTK_RADIO_BUTTON(url->send_normal),
				"To Contact List");
	gtk_box_pack_start(GTK_BOX(h_box), url->send_list, FALSE, FALSE, 0);

	gtk_table_attach(GTK_TABLE(table), h_box, 1, 2, 2, 3,
			 GtkAttachOptions(GTK_FILL | GTK_EXPAND),
			 GTK_FILL, 3, 3); 

	/* If the user is in occ or dnd mode, set the urgent button as def */
	if(url->user->Status() == ICQ_STATUS_DND ||
	   url->user->Status() == ICQ_STATUS_OCCUPIED)
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(url->send_urgent),
				            TRUE);

	/* New hbox */
	h_box = gtk_hbox_new(FALSE, 5);
 
	/* Make the buttons */
	close = gtk_button_new_with_label("Close");
	gtk_signal_connect(GTK_OBJECT(close), "clicked",
			   GTK_SIGNAL_FUNC(url_close), url);

	url->send = gtk_button_new_with_label("Send");
	gtk_signal_connect(GTK_OBJECT(url->send), "clicked",
			   GTK_SIGNAL_FUNC(url_send), url);

	url->cancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(url->cancel), "clicked",
			   GTK_SIGNAL_FUNC(url_cancel), url);
//	gtk_widget_set_sensitive(url->cancel, FALSE);

	gtk_box_pack_start(GTK_BOX(h_box), url->send, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(h_box), url->cancel, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(h_box), close, TRUE, TRUE, 5);

	gtk_table_attach(GTK_TABLE(table), h_box, 1, 2, 4, 5,
			 GTK_FILL, GTK_FILL, 3, 3);

	/* The status bar */
	statusbar = gtk_statusbar_new();
	gtk_table_attach(GTK_TABLE(table), statusbar, 0, 2, 5, 6,
			 GTK_FILL, GTK_FILL, 3, 3);

	guint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "sta");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar), id);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), id, buffer);

	/* Take care of the stuff for the structure */
	url->etag->statusbar = statusbar;
	strcpy(url->etag->buf, buffer);

	/* Catch the text-pushed signal to close the window if needed */
	gtk_signal_connect(GTK_OBJECT(statusbar), "text-pushed",
			   GTK_SIGNAL_FUNC(url_verified_close), url);
	
	/* Show all the widgets at once */
	gtk_widget_show_all(url->window);

	gtk_widget_grab_focus(url->entry_u);
}

void url_send(GtkWidget *widget, struct send_url *url)
{
	/* Enable cancel, disable send */
//	gtk_widget_set_sensitive(url->send, FALSE);
//	gtk_widget_set_sensitive(url->cancel, TRUE);
	
	gchar temp[60];

	const char *url_to_send = gtk_entry_get_text(GTK_ENTRY(url->entry_u));
	const char *desc = gtk_entry_get_text(GTK_ENTRY(url->entry_d));
	gboolean urgent = FALSE;

	/* No pop ups */
	if((url->user->Status() == ICQ_STATUS_DND ||
	    url->user->Status() == ICQ_STATUS_OCCUPIED) &&
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_normal)))
		urgent = TRUE;	

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_urgent)) ||
	   urgent)
	{
		url->etag->e_tag =
			icq_daemon->icqSendUrl(url->user->Uin(), url_to_send, desc,
       	  	(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_server))),
	  	ICQ_TCPxMSG_URGENT);
	}

	/* Send to contact list */
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_list)))
	{
		url->etag->e_tag = icq_daemon->icqSendUrl(url->user->Uin(),
					url_to_send, desc,
		(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_server))),
					ICQ_TCPxMSG_LIST);
	}

	else /* Just send it normally */
	{
		url->etag->e_tag = icq_daemon->icqSendUrl(url->user->Uin(), 
					url_to_send, desc,
					(!gtk_toggle_button_get_active(
						GTK_TOGGLE_BUTTON(url->send_server))),
					ICQ_TCPxMSG_NORMAL);
	}

	strcpy(temp, "Sending URL ");

	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(url->send_server)))
		strcat(temp, "(direct) .. ");
	else
		strcat(temp, "(server) .. ");

	guint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(url->etag->statusbar), "sta");
	gtk_statusbar_pop(GTK_STATUSBAR(url->etag->statusbar), id);
	gtk_statusbar_push(GTK_STATUSBAR(url->etag->statusbar), id, temp);

	/* Take care of the url's etag buffer */
	strcpy(url->etag->buf, "");
	strcpy(url->etag->buf, temp);

	/* The event catcher list */
	catcher = g_slist_append(catcher, url->etag);
}

void url_cancel(GtkWidget *cancel, struct send_url *url)
{
//	gtk_widget_set_sensitive(cancel, FALSE);
//	gtk_widget_set_sensitive(url->send, TRUE);

	/* Cancel the url */
	if(url->etag->e_tag == 0)
		return;

	icq_daemon->CancelEvent(url->etag->e_tag);

	catcher = g_slist_remove(catcher, url->etag);

	gtk_widget_destroy(url->window);
}

void url_close(GtkWidget *widget, struct send_url *url)
{
	gtk_widget_destroy(url->window);
}

void url_verified_close(GtkWidget *statusbar, guint id,
		  	gchar *text, struct send_url *url)
{
	gchar temp[60];
	strcpy(temp, text);
	g_strreverse(temp);

	if(strncmp(temp, "en", 2) == 0)
		gtk_widget_destroy(url->window);
	
	else
	{
//		gtk_widget_set_sensitive(url->send, TRUE);
//		gtk_widget_set_sensitive(url->cancel, FALSE);
	}
}

void list_delete_user(GtkWidget *widget, ICQUser *user)
{
	GtkWidget *yes;
	GtkWidget *no;
	GtkWidget *label;
	GtkWidget *h_box;
	GtkWidget *v_box;
	struct delete_user *d = g_new0(struct delete_user, 1);

	d->user = user;

	const gchar *text = g_strdup_printf("Are you sure you want to delete\n%s (UIN: %ld)\nfrom your list?", d->user->GetAlias(), d->user->Uin());

	/* Make the dialog window */
	d->window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(d->window), "Licq - Delete User");
	gtk_window_set_position(GTK_WINDOW(d->window), GTK_WIN_POS_CENTER);

	/* The main box */
	v_box = gtk_vbox_new(FALSE, 5);

	/* The first box */
	h_box = gtk_hbox_new(FALSE, 5);
	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(h_box), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v_box), h_box, TRUE, TRUE, 5);

	/* The second box, with buttons */
	h_box = gtk_hbox_new(FALSE, 10);
	yes = gtk_button_new_with_label("Yes");
	gtk_box_pack_start(GTK_BOX(h_box), yes, TRUE, TRUE, 0);
	no = gtk_button_new_with_label("No");
	gtk_box_pack_start(GTK_BOX(h_box), no, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v_box), h_box, TRUE, TRUE, 5);

	/* Connect the signals */
	gtk_signal_connect(GTK_OBJECT(no), "clicked",
			   GTK_SIGNAL_FUNC(dialog_close), d->window);
	gtk_signal_connect(GTK_OBJECT(d->window), "destroy",
			   GTK_SIGNAL_FUNC(dialog_close), d->window);
	gtk_signal_connect(GTK_OBJECT(yes), "clicked",
			   GTK_SIGNAL_FUNC(delete_user_callback), d);

	/* Show the widgets */
	gtk_container_add(GTK_CONTAINER(d->window), v_box);
	gtk_widget_show_all(d->window);
}

void delete_user_callback(GtkWidget *widget, struct delete_user *d)
{
	icq_daemon->RemoveUserFromList(d->user->Uin());
	gtk_widget_destroy(d->window);
}

void destroy_dialog(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(widget);
}