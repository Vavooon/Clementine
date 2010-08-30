/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QSettings>

#include "wiimotedev/shortcuts.h"

const char* WiimotedevShortcuts::kActionsGroup = "WiimotedevActions";
const char* WiimotedevShortcuts::kSettingsGroup = "WiimotedevSettings";

WiimotedevShortcuts::WiimotedevShortcuts(QObject* parent)
 :QObject(parent),
  player_(qobject_cast<Player*>(parent)),
  wiimotedev_active_(true),
  wiimotedev_buttons_(0),
  wiimotedev_device_(1),
  wiimotedev_enable_(true),
  wiimotedev_focus_(false),
  wiimotedev_iface_(NULL),
  wiimotedev_notification_(true)
{
  ReloadSettings();
}

void WiimotedevShortcuts::SetWiimotedevInterfaceActived(bool actived) {
  if (!QDBusConnection::systemBus().isConnected())
    return;

  if (actived && !wiimotedev_iface_) {
    wiimotedev_iface_.reset(new DBusDeviceEventsInterface(
        WIIMOTEDEV_DBUS_SERVICE_NAME, WIIMOTEDEV_DBUS_EVENTS_OBJECT,
        QDBusConnection::systemBus(), this));

    connect(wiimotedev_iface_.get(), SIGNAL(dbusWiimoteGeneralButtons(quint32,quint64)),
            this, SLOT(DbusWiimoteGeneralButtons(quint32, quint64)));
  }

  if (!actived && wiimotedev_iface_)
    wiimotedev_iface_.reset();
}

void WiimotedevShortcuts::ReloadSettings() {
  settings_.beginGroup(WiimotedevShortcuts::kActionsGroup);
  settings_.sync();
  actions_.clear();

  if (!settings_.allKeys().count()) {
    RestoreSettings();
    settings_.sync();
  }

  quint64 fvalue, svalue;
  bool fvalid, svalid;

  foreach (const QString& str, settings_.allKeys()) {
    fvalue = str.toULongLong(&fvalid, 10);
    svalue = settings_.value(str, 0).toULongLong(&svalid);
    if (fvalid && svalid) actions_[fvalue] = svalue;
  }

  settings_.endGroup();

  settings_.beginGroup(WiimotedevShortcuts::kSettingsGroup);
  wiimotedev_enable_ = settings_.value("enabled", wiimotedev_enable_).toBool();
  wiimotedev_device_ = settings_.value("device", wiimotedev_device_).toInt();
  wiimotedev_active_ = settings_.value("use_active_action", wiimotedev_active_).toBool();
  wiimotedev_focus_ = settings_.value("only_when_focused", wiimotedev_focus_).toBool();
  wiimotedev_notification_ = settings_.value("use_notification", wiimotedev_notification_).toBool();

  settings_.endGroup();

  SetWiimotedevInterfaceActived(wiimotedev_enable_);
}

void WiimotedevShortcuts::RestoreSettings()
{
  QSettings settings;
  settings.beginGroup(WiimotedevShortcuts::kActionsGroup);
  settings.remove("");
  settings.setValue(QString::number(WIIMOTE_BTN_LEFT), PlayerPreviousTrack);
  settings.setValue(QString::number(WIIMOTE_BTN_RIGHT), PlayerNextTrack);
  settings.setValue(QString::number(WIIMOTE_BTN_SHIFT_LEFT), PlayerPreviousTrack);
  settings.setValue(QString::number(WIIMOTE_BTN_SHIFT_RIGHT), PlayerNextTrack);
  settings.setValue(QString::number(WIIMOTE_BTN_PLUS), PlayerIncVolume);
  settings.setValue(QString::number(WIIMOTE_BTN_MINUS), PlayerDecVolume);
  settings.setValue(QString::number(WIIMOTE_BTN_1), PlayerTogglePause);
  settings.setValue(QString::number(WIIMOTE_BTN_2), PlayerShowOSD);
  settings.endGroup();

  settings.beginGroup(WiimotedevShortcuts::kSettingsGroup);
  settings.remove("");
  settings.setValue("enabled", false);
  settings.setValue("device", 1);
  settings.setValue("use_active_action", true);
  settings.setValue("only_when_focused", false);
  settings.setValue("use_notification", true);
  settings.endGroup();
}

void WiimotedevShortcuts::DbusWiimoteGeneralButtons(quint32 id, quint64 value) {
  if (id != wiimotedev_device_ || !wiimotedev_enable_ || !player_) return;

  quint64 buttons = value & ~(WIIMOTE_TILT_MASK | NUNCHUK_TILT_MASK);

  if (wiimotedev_buttons_ == buttons) return;

  if (actions_.contains(buttons)) {
    switch (actions_.value(buttons, 0xff)) {
      case PlayerNextTrack: player_->Next(); break;
      case PlayerPreviousTrack: player_->Previous(); break;
      case PlayerPlay: player_->Play(); break;
      case PlayerStop: player_->Stop(); break;
      case PlayerIncVolume: player_->VolumeUp(); break;
      case PlayerDecVolume: player_->VolumeDown(); break;
      case PlayerMute: player_->Mute(); break;
      case PlayerPause: player_->Pause(); break;
      case PlayerTogglePause: player_->PlayPause(); break;
      case PlayerSeekBackward: player_->SeekBackward(); break;
      case PlayerSeekForward: player_->SeekForward(); break;
      case PlayerStopAfter: player_->Stop(); break;
      case PlayerShowOSD: player_->ShowOSD(); break;
    }
  }

  wiimotedev_buttons_ = buttons;
}
