// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "remoteinput.h"

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTimer>
#include <QUdpSocket>
#include <QDebug>

RemoteInput::RemoteInput(quint16 port, QObject *parent)
	: QObject(parent),
	socket(new QUdpSocket(this)),
	deadman_timer(new QTimer(this)),
	listening(false),
	has_last_seq(false),
	last_seq(0)
{
	chiaki_controller_state_set_idle(&state);

	deadman_timer->setSingleShot(true);
	deadman_timer->setInterval(DEADMAN_MS);
	connect(deadman_timer, &QTimer::timeout, this, &RemoteInput::DeadmanTimeout);

	// Localhost only: other machines on the network must never reach this socket.
	listening = socket->bind(QHostAddress::LocalHost, port);
	if(!listening)
	{
		qWarning() << "RemoteInput: failed to bind 127.0.0.1:" << port << socket->errorString();
		return;
	}
	qInfo() << "RemoteInput: listening on 127.0.0.1:" << port;
	connect(socket, &QUdpSocket::readyRead, this, &RemoteInput::ReadPending);
}

void RemoteInput::ReadPending()
{
	bool changed = false;
	// Drain everything pending and keep only the newest accepted packet,
	// so a burst of datagrams never turns into accumulated latency.
	while(socket->hasPendingDatagrams())
	{
		QNetworkDatagram datagram = socket->receiveDatagram();
		QByteArray data = datagram.data();
		ChiakiRemoteInputPacket pkt;
		if(chiaki_remote_input_parse(reinterpret_cast<const uint8_t *>(data.constData()), (size_t)data.size(), &pkt) != CHIAKI_ERR_SUCCESS)
			continue;
		if(!chiaki_remote_input_accept_seq(has_last_seq, last_seq, pkt.seq))
			continue;
		has_last_seq = true;
		last_seq = pkt.seq;
		chiaki_remote_input_to_state(&pkt, &state);
		changed = true;
	}
	if(changed)
	{
		deadman_timer->start();
		emit StateChanged();
	}
}

void RemoteInput::DeadmanTimeout()
{
	chiaki_controller_state_set_idle(&state);
	// A restarted sender begins with seq 0 again; accept it.
	has_last_seq = false;
	emit StateChanged();
}
