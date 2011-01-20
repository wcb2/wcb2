#define JABBER_HANDLER_GET_REPLY(x) 	static void x(session_t *s, xmlnode_t *n, const char *from, const char *id)


/**
 * send_error()
 *
 * Sends <service-unavailable/> when we don't want to answer
 *
 */

static void send_error( jabber_private_t * j, char *id, char *to) {
	watch_write(j->send_watch, "<iq id=\"%s\" to=\"%s\" type=\"error\">"
									"<error type='cancel'>"
										"<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
									"</error>"
								"</iq>", id, to);
}

/**
 * jabber_handle_iq_get_disco()
 *
 * Handler for IQ GET QUERY xmlns="http://jabber.org/protocol/disco#items"<br>
 * Send some info about what ekg2 can do/know with given node [node= in n->atts]<br>
 * XXX info about it in XEP/RFC
 *
 * @todo 	We send here only info about node: http://jabber.org/protocol/commands
 * 		Be more XEP/RFC compilant... return error if node not known, return smth
 * 		what we can do at all. etc. etc.
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_disco) {
	jabber_private_t *j = s->priv;

	if (!xstrcmp(jabber_attr(n->atts, "node"), "http://jabber.org/protocol/commands")) {	/* jesli node commandowe */
		/* XXX, check if $uid can do it */
		watch_write(j->send_watch, 
				"<iq to=\"%s\" type=\"result\" id=\"%s\">"
				"<query xmlns=\"http://jabber.org/protocol/disco#items\" node=\"http://jabber.org/protocol/commands\">"
				"<item jid=\"%s/%s\" name=\"Set Status\" node=\"http://jabber.org/protocol/rc#set-status\"/>"
				"<item jid=\"%s/%s\" name=\"Forward Messages\" node=\"http://jabber.org/protocol/rc#forward\"/>"
				"<item jid=\"%s/%s\" name=\"Set Options\" node=\"http://jabber.org/protocol/rc#set-options\"/>"
				"<item jid=\"%s/%s\" name=\"Set ALL ekg2 Options\" node=\"http://ekg2.org/jabber/rc#ekg-set-all-options\"/>"
				"<item jid=\"%s/%s\" name=\"Manage ekg2 plugins\" node=\"http://ekg2.org/jabber/rc#ekg-manage-plugins\"/>"
				"<item jid=\"%s/%s\" name=\"Manage ekg2 plugins\" node=\"http://ekg2.org/jabber/rc#ekg-manage-sessions\"/>"
				"<item jid=\"%s/%s\" name=\"Execute ANY command in ekg2\" node=\"http://ekg2.org/jabber/rc#ekg-command-execute\"/>"
				"</query></iq>", from, id, 
				s->uid+5, j->resource, s->uid+5, j->resource, 
				s->uid+5, j->resource, s->uid+5, j->resource,
				s->uid+5, j->resource, s->uid+5, j->resource,
				s->uid+5, j->resource);
		return;
	}
	/* XXX, tutaj jakies ogolne informacje co umie ekg2 */
}

/**
 * jabber_handle_iq_get_disco_info()
 *
 * Handler for IQ GET QUERY xmlns="http://jabber.org/protocol/disco#info"<br>
 *
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_disco_info) {
	jabber_private_t *j = s->priv;

	watch_write(j->send_watch, "<iq to=\"%s\" type=\"result\" id=\"%s\">"
			"<query xmlns=\"http://jabber.org/protocol/disco#info\">"
				"<feature var=\"jabber:iq:last\"/>"						/* jabber_handle_iq_get_last() */
				"<feature var=\"jabber:iq:version\"/>"					/* jabber_handle_iq_get_version() */
				"<feature var=\"urn:xmpp:ping\"/>"						/* jabber_handle_iq_ping() */
				"<feature var=\"urn:xmpp:time\"/>"                      /* jabber_handle_iq_time() */
				"<feature var=\"http://jabber.org/protocol/chatstates\"/>"
			"</query></iq>", from, id);
#if 0
				"<feature var=\"http://jabber.org/protocol/commands\"/>"
				"<feature var=\"http://jabber.org/protocol/bytestreams\"/>"
				"<feature var=\"http://jabber.org/protocol/si\"/>"
				"<feature var=\"http://jabber.org/protocol/si/profile/file-transfer\"/>"
#endif

}

/**
 * jabber_handle_iq_get_last()
 *
 * Sends reply about our last activity.<br>
 *
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_last) {
	jabber_private_t *j = s->priv;

	int sent = session_int_get(s, "iq_last");
	
	if (!sent) {
		send_error(j, id, from);
		return;
	}

	watch_write(j->send_watch, 
			"<iq to=\"%s\" type=\"result\" id=\"%s\">"
			"<query xmlns=\"jabber:iq:last\" seconds=\"%d\">"
			"</query></iq>", from, id, (time(NULL)-s->activity));
}

/**
 * jabber_handle_iq_get_version()
 *
 * Sends info about our program and system 
 * 
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_version) {
	jabber_private_t *j = s->priv;

	const char *ver_os;
	const char *tmp;

	int sent = session_int_get(s, "iq_version");
	
	if (!sent) {
		send_error(j, id, from);
		return;
	}

	char *escaped_client_name	= jabber_escape(jabberfix((tmp = session_get(s, "ver_client_name")), DEFAULT_CLIENT_NAME));
	char *escaped_client_version	= jabber_escape(jabberfix((tmp = session_get(s, "ver_client_version")), VERSION));
	char *osversion;

	if (!(ver_os = session_get(s, "ver_os"))) {
		struct utsname buf;

		if (uname(&buf) != -1) {
			char *osver = saprintf("%s %s %s", buf.sysname, buf.release, buf.machine);
			osversion = jabber_escape(osver);
			xfree(osver);
		} else {
			osversion = xstrdup(("unknown")); /* uname failed and not ver_os session variable */
		}
	} else {
		osversion = jabber_escape(ver_os);	/* ver_os session variable */
	}

	watch_write(j->send_watch, "<iq to=\"%s\" type=\"result\" id=\"%s\">" 
			"<query xmlns=\"jabber:iq:version\">"
			"<name>%s</name>"
			"<version>%s</version>"
			"<os>%s</os></query></iq>", 
			from, id, 
			escaped_client_name, escaped_client_version, osversion);

	xfree(escaped_client_name);
	xfree(escaped_client_version);
	xfree(osversion);
}

/**
 * jabber_handle_iq_get_ping()
 *
 * Sends answer to ping
 *
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_ping) {
	jabber_private_t *j = s->priv;

	int sent = session_int_get(s, "iq_ping");
	
	if (!sent) { 
		send_error(j, id, from);
		return;
	}

	watch_write(j->send_watch, "<iq to=\"%s\" id=\"%s\" type=\"result\"/>\n",
			from, id);
}

/**
 * jabber_handle_iq_get_ping()
 *
 * Sends curent time in UTC format
 */

JABBER_HANDLER_GET_REPLY(jabber_handle_iq_get_time) {
	jabber_private_t *j = s->priv;

	time_t   t = time(NULL);
	char     time_str[50];
	char     tz_str[7];
	char     *hours;

	int sent = session_int_get(s, "iq_time");
	
	if (!t || !sent) { 
		send_error(j, id, from);
		return;
	}

	strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
	strftime(tz_str, sizeof(tz_str), "%z", localtime(&t));

	hours = xstrndup(tz_str, 3);

	watch_write(j->send_watch, "<iq type=\"result\" to=\"%s\" id=\"%s\">"
							   		"<time xmlns='urn:xmpp:time'>"
							   			"<tzo>%s:%s</tzo>"
							   			"<utc>%s</utc>"
							   		"</time>"
								"</iq>", from, id, hours, tz_str + 3, time_str);
	xfree(hours);
}

static const struct jabber_iq_generic_handler jabber_iq_get_handlers[] = {
	{ "query",		"jabber:iq:last",				            jabber_handle_iq_get_last },
	{ NULL,			"jabber:iq:version",				        jabber_handle_iq_get_version },
	{ NULL,			"http://jabber.org/protocol/disco#items",	jabber_handle_iq_get_disco },
	{ NULL,			"http://jabber.org/protocol/disco#info",	jabber_handle_iq_get_disco_info },

	{ "ping",		"urn:xmpp:ping",				            jabber_handle_iq_get_ping },
	{ "time",		"urn:xmpp:time",				            jabber_handle_iq_get_time },

	{ "",			NULL,						                NULL }
};

// vim:syn=c
// vim:filetype=c
