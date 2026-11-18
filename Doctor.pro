QT += widgets network multimedia serialport charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Doctor/addmedicinedialog.cpp \
    Doctor/appointmentlistpage.cpp \
    Doctor/diagnosisarchivepage.cpp \
    Doctor/doc_mainwindow.cpp \
    Doctor/doctorschedulewidget.cpp \
    Doctor/profilewidget.cpp \
    Doctor/tonguereportdialog.cpp \
    Doctor/uploadreporttask.cpp \
    Staff/adddepartmentdialog.cpp \
    Staff/autoschedule.cpp \
    Staff/deptmanagerwidget.cpp \
    Staff/doctoreditdialog.cpp \
    Staff/doctormanagementwidget.cpp \
    Staff/editscheduledialog.cpp \
    Staff/monthscheduleview.cpp \
    Staff/previewscheduledialog.cpp \
    Staff/schedulemanagepage.cpp \
    Staff/staff_mainwindow.cpp \
    Staff/syslogwidget.cpp \
    Staff/weekscheduleview.cpp \
    Staff/workloadstatswidget.cpp \
    appconfig.cpp \
    commonutil.cpp \
    login_dialog.cpp \
    main.cpp \
    netconn.cpp \
    staff_info.cpp \
    winmanager.cpp

HEADERS += \
    Doctor/addmedicinedialog.h \
    Doctor/appointmentlistpage.h \
    Doctor/diagnosisarchivepage.h \
    Doctor/doc_mainwindow.h \
    Doctor/doctorschedulewidget.h \
    Doctor/profilewidget.h \
    Doctor/tonguereportdialog.h \
    Doctor/uploadreporttask.h \
    Staff/adddepartmentdialog.h \
    Staff/autoschedule.h \
    Staff/deptmanagerwidget.h \
    Staff/doctoreditdialog.h \
    Staff/doctormanagementwidget.h \
    Staff/editscheduledialog.h \
    Staff/monthscheduleview.h \
    Staff/previewscheduledialog.h \
    Staff/schedulemanagepage.h \
    Staff/staff_mainwindow.h \
    Staff/syslogwidget.h \
    Staff/weekscheduleview.h \
    Staff/workloadstatswidget.h \
    appconfig.h \
    commonutil.h \
    dataTypes.h \
    login_dialog.h \
    netconn.h \
    protocol.h \
    staff_info.h \
    winmanager.h

FORMS += \
    Doctor/addmedicinedialog.ui \
    Doctor/diagnosisarchivepage.ui \
    Doctor/doc_mainwindow.ui\
    Doctor/profilewidget.ui \
    Staff/adddepartmentdialog.ui \
    Staff/deptmanagerwidget.ui \
    Staff/doctoreditdialog.ui \
    Staff/doctormanagementwidget.ui \
    Staff/editscheduledialog.ui \
    Staff/schedulemanagepage.ui \
    Staff/staff_mainwindow.ui \
    Staff/syslogwidget.ui \
    login_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES += \
    .gitignore
