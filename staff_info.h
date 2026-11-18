#ifndef STAFF_INFO_H
#define STAFF_INFO_H

#include <QString>

class Staff_Info
{
public:
    Staff_Info();

    QString getName() const;
    void setName(const QString &newName);

    QString getPhone() const;
    void setPhone(const QString &newPhone);

    int getStaff_id() const;
    void setStaff_id(int newStaff_id);

    int getAge() const;
    void setAge(int newAge);

private:
    int staff_id;
    int age;
    QString name;
    QString phone;
};

#endif // STAFF_INFO_H
