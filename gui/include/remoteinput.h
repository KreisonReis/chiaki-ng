// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_REMOTEINPUT_H
#define CHIAKI_GUI_REMOTEINPUT_H

#include <chiaki/controller.h>
#include <chiaki/remoteinput.h>

#include <QObject>

class QUdpSocket;
class QTimer;

/**
 * Receives controller state from an external process (fila-gta home agent)
 * as UDP datagrams on 127.0.0.1:<port>. See chiaki/remoteinput.h for the format.
 *
 * If no valid packet arrives for DEADMAN_MS, the state goes back to idle so
 * no button stays stuck when the sender dies.
 */
class RemoteInput : public QObject
{
	Q_OBJECT

	public:
		static constexpr int DEADMAN_MS = 300;

		RemoteInput(quint16 port, QObject *parent = nullptr);
		bool IsListening() const { return listening; }
		ChiakiControllerState State() const { return state; }

	signals:
		void StateChanged();

	private:
		void ReadPending();
		void DeadmanTimeout();

		QUdpSocket *socket;
		QTimer *deadman_timer;
		ChiakiControllerState state;
		bool listening;
		bool has_last_seq;
		uint16_t last_seq;
};

#endif // CHIAKI_GUI_REMOTEINPUT_H
