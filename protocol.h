#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

enum SERVICE_TYPE
{
    PATIENT_LOGIN,   //患者端登录(卡密登录)
    PHONE_LOGIN, //手机号登录
    VERIFY_CODE, //请求验证码
    INFO_NETRY, //信息录入
    FILE_UPLOAD, // 图片上传
    SAVE_TONGUE_RECORD, //检测记录保存

    STAFF_LOGIN,    //工作人员登录
    WEEK_SCHEDULE, //周排班
    DOCTOR_LIST, //医生列表
    DEPARTMENT_LIST, //科室列表
    Schedule_Doc, //排班医生信息
    CHECK_ADD_SCHEDULE, //检查添加排班医生
    CHECK_DEL_SCHEDULE, //删除排班医生
    MONTH_SCHEDULE, //月排班
    AUTO_SCHEUDLE, //自动排班
    AUTO_APPOINTMENT_QUERY, //最近排班
    AUTO_APPOINTMENT_CREATE, //添加预约记录

    DOCTOR_SCHEDULE_DATES, // 排班医生的日期
    AVAILABLE_SHIFTS, //可预约时段

    APPOINTMENT_LIST, // 查预约列表
    APPOINTMENT_MISS, // 标记失约
    TONGUE_RECORD_DETAIL, //检测报告
    TONGUE_IMAGE_CHUNK, //检测报告图片文件

    PATIENT_INFO,        // 患者基本信息
    HISTORY_REPORT,      // 历史报告
    SAVE_DIAGNOSIS,      // 保存诊疗记录
    GENERATE_REPORT,     // 生成报告
    ALL_HISTORY_RECORD, //全部历史诊疗记录
    VIEW_RECORD, //查看诊疗记录详情
    PATIENT_APPOINTMENT_LIST, //查询预约列表（ALL)
    MODIFYPWD, //修改密码
    DOCTOR_WEEK_SCHEDULE, //医生周排班

// 系统日志管理模块
    GET_LOG_LIST, //获取日志列表

// 科室档案管理模块
    GET_DEPT_LIST, //查询科室档案
    GET_CHIEF_DOCTORS,       // 获取所有主任/副主任医师
    GET_UNASSIGNED_DOCTORS,  // 获取所有未分配科室的医生
    ADD_DEPARTMENT,          // 新增科室
    GET_DEPT_DETAIL, //查询科室信息
    UPDATE_DEPT, //更新科室

// 诊疗工作量模块
    GET_WORKLOAD_STATS, // 获取诊疗工作量统计

// 医师档案管理
    GET_ALL_DOCTORS, // 请求所有医生列表
    UPDATE_DOCTOR, // 修改医生
    DELETE_DOCTOR,  // 删除医生
    GET_NEW_DOCTOR_ID //获取新工号
};

#pragma pack(push, 1)
typedef struct {
    SERVICE_TYPE type;
    int len; //数据包长度
}HEAD;

typedef struct
{
    char work_id[20];
    char password[50];
}STAFF_LOGIN_REQ;

typedef struct
{
    int staff_id;
    char name[20];
    char phone[12];
    int gender;

    char msg[50]; //错误信息
    int role; // 1--医生 2--管理员
    int result;  //响应结果
}STAFF_LOGIN_RESP;


typedef struct
{
    char monday[10];  //周一
    char sunday[10]; //周日
    int deptId; //科室号
}WEEK_SCHEDULE_REQ;

typedef struct
{
    int schedule_id;
    int doc_id;
    char doc_name[23];
    char doc_title[23];
    char workdate[11];
    uint8_t shift;

}WEEK_SCHEDULE_RESP;


typedef struct {
    char first_day[11];
    char last_day[11];
    int  deptId;
} MONTH_SCHEDULE_REQ;

typedef struct {
    int     schedule_id;
    int     doc_id;
    char    doc_name[23];
    char    doc_title[23];
    char    workdate[11];
    uint8_t shift;
} MONTH_SCHEDULE_RESP;

typedef struct
{
    int dept_id;
}DOCTOR_LIST_REQ;

typedef struct
{
    int  staff_id;           // 医生 ID
    //char work_no[16];        // 工号
    char real_name[32];      // 姓名
    char title[16];          // 职称
    //int  dept_id;            // 所属科室
    int  max_weekly_shifts;  // 每周排班上限
}DOCTOR_LIST_RESP;



// 查询所有科室
typedef struct
{
    int reserved; //占位
}DEPARTMENT_LIST_REQ;

typedef struct
{
    int dept_id;
    char dept_name[64];
}DEPARTMENT_LIST_RESP;


// 指定班次医生
typedef struct
{
    uint8_t shift; //班次
    char workdate[11]; //日期
    int deptId; //科室号

}Schedule_Doc_REQ;

typedef struct
{
    int schedule_id;
    int staff_id;
    char name[23];
    char title[23];

}Schedule_Doc_RESP;


typedef struct
{
    int dept_id;
    int staff_id;
    uint8_t shift;
    int force; // 0--先检查
    char workdate[11]; //日期

}ADD_SCHEDULE_REQ;

typedef struct
{
    int  code;        // 0=通过, 1=软冲突, 2=硬冲突
    char message[128];// 具体冲突说明

}ADD_SCHEDULE_RESP;



typedef struct {
    int     dept_id;
    int     doctor_id;      // 要删除的医生
    uint8_t shift;
    char    workdate[11];
} DEL_SCHEDULE_REQ;

typedef struct {
    int  code;              // 0=成功，其他=错误
    char message[128];
} DEL_SCHEDULE_RESP;


typedef struct
{
    int dept_id;
    int count;

}AUTO_SCHEDULE_BATCH_REQ;

typedef struct {
    int staff_id;
    char workdate[11];
    uint8_t shift;
}AUTO_SCHEDULE_ITEM;

typedef struct
{
    int code;
    //int successCount;
    // int failCount;
    char msg[256];

}AUTO_SCHEDULE_RESP;

typedef struct {
    int patient_id;
    char image_url[256];
    int result_type;
    int conf;
    char device_id[16];
} SAVE_TONGUE_RECORD_REQ;

typedef struct {
    int result;
    int record_id;
    char msg[50]; //错误信息
} SAVE_TONGUE_RECORD_RESP;

// 查询最近排班请求
typedef struct {
    int  patient_id;
    int  tongue_record_id;
} AUTO_APPOINTMENT_QUERY_REQ;

// 查询最近排班响应
typedef struct {
    int     code;
    int     schedule_id;
    int     doctor_id;
    int     dept_id;
    char    doctor_name[32];
    char    doctor_title[16];
    char    dept_name[64];
    char    work_date[11];
    uint8_t shift_type;
    char    message[128];
} AUTO_APPOINTMENT_QUERY_RESP;


// 创建自动预约请求
typedef struct {
    int     patient_id;
    int     schedule_id;
    int     tongue_record_id;
    int     doctor_id;
    int     dept_id;
    char    work_date[11];
    uint8_t shift_type;
} AUTO_APPOINTMENT_CREATE_REQ;

// 创建自动预约响应
typedef struct {
    int     code;
    int     appointment_id;
    char    message[128];
} AUTO_APPOINTMENT_CREATE_RESP;


typedef struct {
    int doctor_id;
} DOCTOR_SCHEDULE_DATES_REQ;

typedef struct {
    char work_date[11];     // "yyyy-MM-dd"
} DOCTOR_SCHEDULE_DATES_ITEM;

typedef struct {
    int  doctor_id;
    char work_date[11];
} AVAILABLE_SHIFTS_REQ;

typedef struct {
    int     schedule_id;
    uint8_t shift_type;     // 1/2/3
    int     remaining;      // 剩余可预约数（可选，用于显示"已满"）
} AVAILABLE_SHIFTS_ITEM;




// ===============================================患者预约记录管理==============================================================
// 一、患者预约记录 -- 指定班次
typedef struct {
    int doctor_id;
    char work_date[11];  // 今日日期
    int  shift_type;  //传入-1 即查看本日所有预约信息
    int page;   // 当前所在页
    int page_size;  // 每页容纳记录数
} APPOINTMENT_LIST_REQ;

// 响应头
typedef struct {
    int code;
    int count;   // 本页预约数
    int total;   // 总预约数
} APPOINTMENT_LIST_RESP_HEAD;

// 单条预约
typedef struct {
    int     appointment_id;
    int     patient_id;
    char    patient_name[32];
    char    work_date[11];
    uint8_t shift_type;
    int     tongue_record_id;   // -1 表示无报告
    int     status;             // 1=已预约 2=已问诊 3=已失约 4=已取消
} APPOINTMENT_LIST_RESP;

// 二、失约操作 -- 修改 status = 2
typedef struct {
    int appointment_id;
} APPOINTMENT_MISS_REQ;

typedef struct {
    int  code;
    char message[128];
} APPOINTMENT_MISS_RESP;

// 三、舌苔检测报告 -- 查看报告按钮
typedef struct {
    int tongue_record_id;
} TONGUE_RECORD_DETAIL_REQ;

typedef struct {
    int     code;               // 0=成功
    char    image_url[512];     // 舌苔图片路径
    int     image_size;         //图片文件总大小
    int     chunk_total;        //总分片数
    int     transfer_mode;      // 0--不分片 1--分片

    int     result_type;       // 0=灰黑苔 1=镜面舌 2=薄白苔 3=白腻苔 4=黄腻苔
    double  confidence;        // 0~100
    int     health_status;     // 0=健康 1=异常
    int     is_severe;         // 0=否 1=是
    char    created_at[20];    // "2026-10-11 14:30:00"

    char    message[128];
} TONGUE_RECORD_DETAIL_RESP;

// 分片操作
typedef struct {
    int tongue_record_id;
    int chunk_index; //分片索引
} TONGUE_IMAGE_CHUNK_REQ;

typedef struct {
    int code;
    int chunk_index;
    int chunk_total;
    int chunk_size;
    char chunk_data[10240];
    char msg[64];
} TONGUE_IMAGE_CHUNK_RESP;




// ---- 患者信息 ----
typedef struct {
    int patient_id;
} PATIENT_INFO_REQ;

typedef struct {
    int  code;
    char name[32];
    int  age;
    char phone[12];
    int  gender;
    char message[128];
} PATIENT_INFO_RESP;

// ---- 历史报告 ----
typedef struct {
    int patient_id;
} HISTORY_REPORT_REQ;

typedef struct {
    int     code; // 0--有历史记录 1--无
    int     record_id;
    int     doctor_id;
    char    doctor_name[32];
    char    diagnosis[2048]; // 诊断 + 建议
    char    visit_date[20];
    int     item_count;  //药品数量
} HISTORY_REPORT_RESP;

// ---- 保存诊疗 ----
typedef struct {
    int     appointment_id;      // 关联预约
    int     patient_id;
    int     doctor_id;
    int     tongue_record_id;    // 舌苔报告ID，可 -1

    char    diagnosis[1024];     // 诊疗结果
    char    advice[1024];        // 诊疗建议

    int     item_count;          // 处方明细数
} SAVE_DIAGNOSIS_REQ;

typedef struct {
    char medicine_name[64];
    char dosage[32];
    char usage_method[128];
    uint8_t sort_order;
} PRESCRIPTION_ITEM;

typedef struct {
    int  code;
    int  record_id;        // 保存后的记录 ID
    char message[128];
} SAVE_DIAGNOSIS_RESP;




// ---- 生成报告 ----
typedef struct {
    int medical_record_id;  // 服务器去数据库查询数据  生成pdf文件保存
} GENERATE_REPORT_REQ;

typedef struct {
    int  code;              // 0=成功
    char report_url[512];   // 服务端路径
    char message[128];
} GENERATE_REPORT_RESP;

// 修改密码
typedef struct {
    int staff_id;
    char oldPwd[50];
    char newPwd[50];
} MODIFY_PWD_REQ;

typedef struct {
    int  code;   // 0=成功
    char message[128];
} MODIFY_PWD_RESP;


// 医生个人排班
typedef struct {
    int  doctor_id;
    char start_date[11]; // "2026-04-13" (周一)
    char end_date[11];   // "2026-04-19" (周日)
} DOCTOR_WEEK_SCHEDULE_REQ;

typedef struct {
    char work_date[11];
    uint8_t shift_type;  // 1=上午 2=下午 3=夜班
} DOCTOR_WEEK_SCHEDULE_ITEM;

typedef struct {
    int code;
    int count;
} DOCTOR_WEEK_SCHEDULE_RESP_HEAD;


// 系统日志管理
// 日志列表
typedef struct {
    char search_date[11];
    char search_type[32];
    int  page;
    int  page_size;
} GET_LOG_LIST_REQ;

typedef struct {
    int  log_id;
    char operate_date[11];
    char operator_name[32];
    char operate_type[32];
    char detail[256];
} GET_LOG_ITEM;

typedef struct {
    int code;
    int count;   // 本页条数
    int total;   // 总条数
} GET_LOG_LIST_RESP_HEAD;



// ===============================================科室档案管理==============================================================
// 1、科室信息
typedef struct {
    int page;
    int page_size;
} DEPT_PAGE_REQ;

typedef struct {
    int code;
    int count;   // 本页条数
    int total;   // 总条数
} DEPT_PAGE_RESP_HEAD;

typedef struct {
    int dept_id;
    char dept_name[50];
    char created_date[11];
} DEPT_PAGE_ITEM;

// 一、=============================新增科室====================
// 主任/副主任医师
typedef struct {
    int current_dept_id;
} CHIEF_DOC_REQ;
// 响应包复用  DOCTOR_LIST_RESP

// 未分配医生
typedef struct {
    int current_dept_id;
} EMPTY_DOCTOR_REQ;
// 响应包复用  DOCTOR_LIST_RESP

// 新增科室
typedef struct {
    int leader_id; //负责人ID
    char deptName[50];
    char Profile[50];
    int member_count;
} ADD_DEPT_REQ;

typedef struct {
    int  code;   // 0=成功
    char message[128];
} ADD_DEPT_RESP;


// 二、=============================编辑科室====================
// 1. 获取科室详情
typedef struct {
    int dept_id;
} GET_DEPT_DETAIL_REQ;

typedef struct {
    int  code;
    char dept_name[50];
    char profile[256];
    int  leader_id;
    int  member_count;   // 该科室当前的人数（不含负责人）
    // 注意：包体后面紧跟 member_count 个 int（成员ID）
} GET_DEPT_DETAIL_RESP;

// 2. 更新科室
typedef struct {
    int  dept_id;
    int  leader_id;
    char dept_name[50];
    char profile[256];
    int  member_count;   // 新名单的人数
    // 包体后面紧跟 member_count 个 int（成员ID）
} UPDATE_DEPT_REQ;

typedef struct {
    int  code;
    char message[128];
} UPDATE_DEPT_RESP;



// ===============================================诊疗工作量统计==============================================================
typedef struct {
    int mode; // 1=周, 2=月
} WORKLOAD_STATS_REQ;

// 预约数、就诊数、就诊率、失约率
typedef struct {
    int total_bookings;
    int total_visits;
    double visit_rate;
    double miss_rate;
    double booking_growth;
    double visit_growth;     // 就诊量环比
    double rate_growth;      // 就诊率环比 (百分点差值)
    double miss_rate_growth; // 失约率环比 (百分点差值)
} STATS_SUMMARY;


typedef struct {
    char dept_name[32];
    int current_period;
    int last_period;
} DEPT_VOLUME_ITEM;

typedef struct {
    char dept_name[32];
    double visit_rate;
    double miss_rate;
    double cancel_rate;
} DEPT_RATE_ITEM;

typedef struct {
    char doctor_name[32];
    int visit_count;
} DOCTOR_RANK_ITEM;

typedef struct {
    int mode;
    STATS_SUMMARY summary;
    int dept_volume_count;
    int dept_rate_count;
    int doc_rank_count;
    // 后面紧跟变长数组
} WORKLOAD_STATS_RESP;



// ===============================================医师档案管理==============================================================
// 医生信息
typedef struct  {
    char work_no[16];
    char real_name[32];
    char dept_name[32];
    char title[32];
    int age;
}DOCTOR_ITEM;

// 响应获取列表
typedef struct{
    int code;
}GET_DOCTORS_REQ;

// 响应获取列表
typedef struct{
    int code; // 0成功，1失败
    int count; // 医生数量
}GET_DOCTORS_RESP;

// 新增/修改请求
typedef struct MODIFY_DOCTOR_REQ{
    int action;// 1=新增, 2=修改
    DOCTOR_ITEM doctor;
}MODIFY_DOCTOR_REQ;


typedef struct {
    int code;
    char work_no[16];
    char msg[64];
}NEW_DOCTOR_ID_RESP ;

// 删除请求
typedef struct{
    char work_no[16];
}DELETE_DOCTOR_REQ;

// 通用
typedef struct{
    int code; // 0成功，1失败
    char msg[64];
}SIMPLE_RESP;

#pragma pack(pop)

#endif
