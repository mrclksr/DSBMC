/*-
 * Copyright (c) 2019 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <QMutex>
#include <QThread>

#include "libdsbmc.h"

class Thread : public QThread {
  Q_OBJECT
 public:
  Thread(QMutex *mutex, int command, dsbmc_t *handle, const dsbmc_dev_t *dev,
         bool force = false, int speed = 1, QObject *parent = 0);
  Thread(QMutex *mutex, int command, dsbmc_t *handle, const dsbmc_dev_t *dev,
         QString program, QObject *parent = 0);
 signals:
  void commandReturned(int, const dsbmc_dev_t *, int);
  void commandReturned(int, const dsbmc_dev_t *, int, QString);

 protected:
  void run();

 public:
  int speed;
  int command;
  bool force;
  QMutex *mutex;
  dsbmc_t *handle;
  QString program;
  const dsbmc_dev_t *dev;
};
