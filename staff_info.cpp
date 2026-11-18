#include "staff_info.h"

Staff_Info::Staff_Info() {}

QString Staff_Info::getName() const
{
    return name;
}

void Staff_Info::setName(const QString &newName)
{
    name = newName;
}


QString Staff_Info::getPhone() const
{
    return phone;
}

void Staff_Info::setPhone(const QString &newPhone)
{
    phone = newPhone;
}

int Staff_Info::getStaff_id() const
{
    return staff_id;
}

void Staff_Info::setStaff_id(int newStaff_id)
{
    staff_id = newStaff_id;
}

int Staff_Info::getAge() const
{
    return age;
}

void Staff_Info::setAge(int newAge)
{
    age = newAge;
}
