// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

namespace demo {
enum class State { Offered, Waiting, Connecting, Transferring, Complete,
                   Failed, Cancelled, Rejected };
inline QString stateText(State state) {
  switch (state) {
  case State::Offered: return QStringLiteral("等待你接受");
  case State::Waiting: return QStringLiteral("等待对端接受");
  case State::Connecting: return QStringLiteral("正在连接");
  case State::Transferring: return QStringLiteral("正在传输");
  case State::Complete: return QStringLiteral("演示完成 · 未实际传输");
  case State::Failed: return QStringLiteral("连接失败 · 可重试");
  case State::Cancelled: return QStringLiteral("已取消");
  case State::Rejected: return QStringLiteral("已拒绝");
  }
  return {};
}
inline bool active(State s) {
  return s == State::Offered || s == State::Waiting || s == State::Connecting ||
         s == State::Transferring;
}
inline bool retryable(State s) {
  return s == State::Failed || s == State::Cancelled || s == State::Rejected;
}
struct Transfer {
  QString file, peer;
  bool incoming = false;
  State state = State::Waiting;
  int progress = 0;
  QStringList files;
  qint64 bytes = 0;
};
class Model {
public:
  QVector<Transfer> jobs;
  int add(const QString &file, const QString &peer, bool incoming = false,
          const QStringList &files = {}, qint64 bytes = 0) {
    if (file.trimmed().isEmpty() || peer.trimmed().isEmpty()) return -1;
    jobs.push_back({file, peer, incoming,
                   incoming ? State::Offered : State::Waiting, 0,
                   files.isEmpty() ? QStringList{file} : files, bytes});
    return jobs.size() - 1;
  }
  bool accept(int id) {
    if (!valid(id)) return false;
    auto &t = jobs[id];
    if (t.state != State::Offered && t.state != State::Waiting) return false;
    t.state = State::Connecting;
    return true;
  }
  bool reject(int id) {
    if (!valid(id) || (jobs[id].state != State::Offered &&
                      jobs[id].state != State::Waiting)) return false;
    jobs[id].state = State::Rejected;
    return true;
  }
  bool cancel(int id) {
    if (!valid(id) || !active(jobs[id].state)) return false;
    jobs[id].state = State::Cancelled;
    return true;
  }
  bool fail(int id) {
    if (!valid(id) || !active(jobs[id].state)) return false;
    jobs[id].state = State::Failed;
    return true;
  }
  bool retry(int id) {
    if (!valid(id) || !retryable(jobs[id].state)) return false;
    auto &t = jobs[id];
    t.state = t.incoming ? State::Offered : State::Waiting;
    t.progress = 0;
    return true;
  }
  void tick() {
    for (auto &t : jobs) {
      if (t.state == State::Connecting) t.state = State::Transferring;
      else if (t.state == State::Transferring) {
        t.progress = qMin(100, t.progress + 20);
        if (t.progress == 100) t.state = State::Complete;
      }
    }
  }
private:
  bool valid(int id) const { return id >= 0 && id < jobs.size(); }
};
}
