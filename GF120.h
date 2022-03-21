#ifndef GF120_H
#define GF120_H


struct InfraredBlockbodyStatus{
    unsigned short		user_calibration_enable;	//用户设置区域检测温度功能打开
    unsigned short		user_calibration_temp;		//黑体炉设置温度
    unsigned short		user_width_start;			//用户设置区域检测温度区宽开始点
    unsigned short		user_width_end;				//用户设置区域检测温度区宽结束点
    unsigned short		user_high_start;			//用户设置区域检测温度区高开始点
    unsigned short		user_high_end;				//用户设置区域检测温度区高结束点
};

struct InfraredColorStatus
{
    unsigned short		user_color_enable;				//用户设置 温度微彩范围 三段曲线的中间两个点
    unsigned short		user_color_high;                //用户设置 低温色彩
    unsigned short		user_color_low;                 //用户设置 高温色彩
};

struct InfraredTemperatureROIStatus {
    unsigned short		user_roi_enable;			//用户设置区域检测温度功能打开
    unsigned short		user_width_start;			//用户设置区域检测温度区宽开始点
    unsigned short		user_width_number;			//用户设置区域检测温度区宽像素点个数
    unsigned short		user_high_start;			//用户设置区域检测温度区高开始点
    unsigned short		user_high_number;			//用户设置区域检测温度区高素点个数
    unsigned short		user_roi_emissivity;
};


struct InfraredStatus
{
    char        gf120_dev_version[32];  //sdk的版本号
    unsigned short frame_val;				//指示改帧是否有效，在校正其间传输的是校正前的静止图像
    unsigned short gst417m_temp;			//红外相机焦面温度
    unsigned short gst417m_tec_temp;		//红外相机设置温度
    short outside_temp;           //红外相机设置温度
    short shutter_temp;			//红外相机快门温度
    unsigned short gst417m_ctia;           //相机参数
    int			low_temp;				//最低温值
    unsigned short low_temp_width;			//最低温宽坐标
    unsigned short low_temp_hight;			//最低温高坐标
    int			high_temp;				//最高温值
    unsigned short high_temp_width;		//最高温宽坐标
    unsigned short high_temp_hight;		//最高温高坐标
    int			center_temp;			//图像中心位置温度
    int			user_temp;				//用户设置区域温度
    unsigned short ad_value;
    short diff_ad_value;
    short diff_temp_value;
    char		config_dir[256];		//配置文件目录
    int			calibration_high_temp;				//高温黑体炉温度
    int			calibration_low_temp;				//低温黑体炉温度
    int			gst417m_frame_cnt;					//相机输出帧个数
    int			gst417m_frame_rate;					//统计到的帧率
};

struct InfraredParamsStatus
{
    unsigned short		temp_mode;					//三种测温区间	-40℃~150℃	-40~800℃	20~50℃
    unsigned short		sample_mode;				//样本采样模式
    unsigned short      manual_mode;                //手动模式
    short      manual_temp;                 //手动模式温度
    unsigned short		stop_correct;				//停止校正
    unsigned short		shutter_on;					//快门强制操作
    unsigned short		collect_cover_sample;				//采集锅盖现象样本使用
    short		collect_cover_temp;				//采集锅盖现象样本使用
    unsigned short		collect_response_sample;
    short		collect_response_temp;
    unsigned short		response_rate_sample;		//响应率样本采集 高低温个一个
    unsigned short		color_mode;					//伪彩色编码种类 支持用户调色板 256个RGB色度值
    unsigned short		user_emissivity;			//设置的发射率
    unsigned short		sharpen_grade;				//锐化等级 设置我0 不锐化
    unsigned short		exposure_time;				//曝光时间
    unsigned short		out_rgb888_enable;				//输出8bit伪彩色图形 否则输出16位灰度数据
    unsigned short		osd_enable;					//在输出伪彩色图像时是否添加OSD显示，最高最低温度
    unsigned short		qt_img;						//qt需要RB对调 和上下镜像
    unsigned short		temp_display_enable;					//在输出伪彩色图像时是否添加OSD显示，最高最低温度
    short		user_compensate_temp;                   //用户自定义补偿
    short		humidity_compensate_temp;               //湿度补偿
    short		distance_compensate_temp;               //距离补偿
    short		compensate_temp;                        //三种补偿和
    unsigned short      high_temp_warm_enable;              //高温报警使能
    unsigned short      high_temp_warm_temp;                 //高温报警温度
    unsigned short      low_temp_warm_enable;              //高温报警使能
    unsigned short      low_temp_warm_temp;                 //高温报警温度
    char		file_path[256];                     //低温锅盖采样文件
    char		low_cover_file[256];				//低温锅盖采样文件
    char		high_cover_file[256];				//高温锅盖采样文件
    char		low_response_file[256];				//低温响应率采样文件
    char		high_response_file[256];			//高温响应率采样文件
    InfraredTemperatureROIStatus		GF120_roi;
    InfraredColorStatus         GF120_color;
    InfraredBlockbodyStatus     GF120_calibration;
    unsigned int		frame_temp_cnt;					//多少帧进行一次温度统计 伪彩色使用25帧统计一次 抓图可以这是为1帧统计一
    unsigned int		dde_grade;						//锐化等级 设置我0 不锐化
    unsigned short		frame_rate_set;						//锐化等级 设置我0 不锐化
    unsigned short		ROI_R_COLOR;						//ROI R色彩
    unsigned short		ROI_G_COLOR;						//ROI G色彩
    unsigned short		ROI_B_COLOR;						//ROI B色彩
    unsigned short		cold_mode;                          //非连续模式
};

#endif // GF120_H
