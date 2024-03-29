#include <stdio.h>
#include <math.h>
#include "common.h"
#include "emd.h"


typedef enum _condition
{
	NATURAL,
	CLAMPED,
	NOTaKNOT
}condition;

typedef struct _coefficient
{
	float a3;
	float b2;
	float c1;
	float d0;
}coefficient;

typedef struct _point
{
	coefficient *coe;
	float *xCoordinate;
	float *yCoordinate;
	float f0;
	float fn;
	int num;
	condition con;
}point;

typedef struct _comp  // 复数结构体声明；
{
	float re;
	float im;
}comp;

#define N CALCULATE_SIZE
#define PI 3.141592653
#define freq 25
#define peri 20.48

comp asp[N],hea[N]; //  存储的呼吸和心跳重构信号；
float breath,heart;
static coefficient coe_max[256], coe_min[256];
static float temp_x_max[256], temp_y_max[256], temp_x_min[256], temp_y_min[256];
static float m[N];
static comp ht[N];


int extr_min(float* y, int* b);
int extr_max(float* y, int* b);
void bitrev(comp data[N]);
void fft(comp data[N]);
int spline(point *point);
float func(float a,float b,float c,float d,float x,float y);
int imf(float x[N],float y[N],int b[N],float h[N]);
void emd(float *x, float *y, float fc);
float max_val(comp *biosig);

/***************************极小值***************************/

int extr_min(float* y, int* b)
{
	int j = 0;
	int i;

	//找到所有的极小值，并且将对应的横坐标存入数组中；
	for(i=1; i<N-1; i++)
	{
		if(y[i]<y[i+1]&&y[i]<y[i-1])
		{	
			b[++j] = i;//从第一个开始，第零个准备存原始信号的第一个数；
		}
	}
	return j;  
}

/***************************极大值***************************/
int extr_max(float* y, int* b)
{
	int j = 0; 
	int i;

	//找到所有的极大值，并且将对应的横坐标存入数组中；
	for(i=1; i<N-1; i++)
	{
		if(y[i]>y[i+1]&&y[i]>y[i-1])
		{	
			b[++j] = i;//从第一个开始，第零个准备存原始信号的第一个数；
		}
	}
	return j; 
}

void bitrev(comp data[N])  //fft的蝶形运算之前先进行排序；

{

    int    p=1, q, i;
    int    bit_rev[ N ];
    float   xx_r[ N ];
   
    bit_rev[ 0 ] = 0;

    while( p < N )

    {

       for(q=0; q<p; q++) 
       {

           bit_rev[ q ]     = bit_rev[ q ] * 2;

           bit_rev[ q + p ] = bit_rev[ q ] + 1;

       }

       p *= 2;

    }

	for(i=0; i<N; i++)   xx_r[ i ] = data[i].re;   
	for(i=0; i<N; i++)   data[i].re = xx_r[ bit_rev[i] ]; //xx_r[N]要定义全局变量；

}

void fft(comp data[N])
{
	int L;												  //碟形运算的每级 L = 1, 2,......,M

	int J;												  //J = 0, 1, 2....., 2^(L - 1) - 1

	int B;												  //B = 2^(L - 1)

	int P;												  //P = J * 2^(M - L)

	int M = 9; //  M的值取决于N，M = log2(N);

	int k;

	float datak_re,datak_im,datakb_re,datakb_im;	

	bitrev(data);   //排序；

    //碟形运算;													
	for(L = 1; L <= M; L++)
	{
		B = 1 << (L - 1);
		
		for(J = 0; J <= B - 1; J++)
		{
			P = J * (1 << (M - L));
			
			for(k = J; k <= N - 1; k += (1 << L))
			{	datak_re = data[k].re;
				datak_im = data[k].im;
				datakb_re = data[k+B].re;
				datakb_im = data[k+B].im;
				data[k].re = datak_re + cos(2*PI*P/N)*datakb_re + sin(2*PI*P/N)*datakb_im;
				data[k].im = datak_im + cos(2*PI*P/N)*datakb_im - sin(2*PI*P/N)*datakb_re;
				data[k+B].re = datak_re - cos(2*PI*P/N)*datakb_re - sin(2*PI*P/N)*datakb_im;
				data[k+B].im = datak_im - cos(2*PI*P/N)*datakb_im + sin(2*PI*P/N)*datakb_re;
			}

		}

	}
}

/**********************************spline********************************************/

int spline(point *point)
{
	float *x = (*point).xCoordinate, *y = (*point).yCoordinate;
	int n = (*point).num - 1;
	coefficient *coe = (*point).coe;
	condition con = (*point).con;
	float temp;
	int i;
    float h[256], d[256], a[256], b[256], c[256], f[256];
    float	*m = b;
	/* 计算 h[] d[] */
	for (i = 0; i < n; i++)
	{
		h[i] = x[i + 1] - x[i];
		d[i] = (y[i + 1] - y[i]) / h[i];
	}
	/**********************
	** 初始化系数增广矩阵
	**********************/
	a[0] = (*point).f0;
	c[n] = (*point).fn;
	/* 计算 a[] b[] d[] f[] */
	switch(con)
	{
		case NATURAL:
			f[0] = a[0];
			f[n] = c[n];
			a[0] = 0;
			c[n] = 0;
			c[0] = 0;
			a[n] = 0;
			b[0] = 1;
			b[n] = 1;
		break;
		case CLAMPED:
			f[0] = 6 * (d[0] - a[0]);
			f[n] = 6 * (c[n] - d[n - 1]);
			a[0] = 0;
			c[n] = 0;
			c[0] = h[0];
			a[n] = h[n - 1];
			b[0] = 2 * h[0];
			b[n] = 2 * h[n - 1];
		break;
		case NOTaKNOT:
			f[0] = 0;
			f[n] = 0;
			a[0] = h[0];
			c[n] = h[n - 1];
			c[0] = -(h[0] + h[1]);
			a[n] = -(h[n - 2] + h[n - 1]);
			b[0] = h[1];
			b[n] = h[n - 2];
		break;
	}
	for (i = 1; i < n; i++)
	{
		a[i] = h[i - 1];
		b[i] = 2 * (h[i - 1] + h[i]);
		c[i] = h[i];
		f[i] = 6 * (d[i] - d[i - 1]);
	}
	/*************
    ** 高斯消元
    *************/
    /* 第0行到第(n-3)行的消元 */
    for(i = 0; i <= n - 3; i++)
    {
        /* 选主元 */
        if( fabs(a[i + 1]) > fabs(b[i]) )
        {
            temp = a[i + 1]; a[i + 1] = b[i]; b[i] = temp;
            temp = b[i + 1]; b[i + 1] = c[i]; c[i] = temp;
            temp = c[i + 1]; c[i + 1] = a[i]; a[i] = temp;
            temp = f[i + 1]; f[i + 1] = f[i]; f[i] = temp;
        }
        temp = a[i + 1] / b[i];
        a[i + 1] = 0;
        b[i + 1] = b[i + 1] - temp * c[i];
        c[i + 1] = c[i + 1] - temp * a[i];
        f[i + 1] = f[i + 1] - temp * f[i];
    }
    
    /* 最后3行的消元 */
    /* 选主元 */
    if( fabs(a[n - 1]) > fabs(b[n - 2]) )
    {
        temp = a[n - 1]; a[n - 1] = b[n - 2]; b[n - 2] = temp;
        temp = b[n - 1]; b[n - 1] = c[n - 2]; c[n - 2] = temp;
        temp = c[n - 1]; c[n - 1] = a[n - 2]; a[n - 2] = temp;
        temp = f[n - 1]; f[n - 1] = f[n - 2]; f[n - 2] = temp;
    }
    /* 选主元 */
    if( fabs(c[n]) > fabs(b[n - 2]) )
    {
        temp = c[n]; c[n] = b[n - 2]; b[n - 2] = temp;
        temp = a[n]; a[n] = c[n - 2]; c[n - 2] = temp;
        temp = b[n]; b[n] = a[n - 2]; a[n - 2] = temp;
        temp = f[n]; f[n] = f[n - 2]; f[n - 2] = temp;
    }
    /* 第(n-1)行消元 */
    temp = a[n - 1] / b[n - 2];
    a[n - 1] = 0;
    b[n - 1] = b[n - 1] - temp * c[n - 2];
    c[n - 1] = c[n - 1] - temp * a[n - 2];
    f[n - 1] = f[n - 1] - temp * f[n - 2];
    /* 第n行消元 */
    temp = c[n] / b[n - 2];
    c[n] = 0;
    a[n] = a[n] - temp * c[n - 2];
    b[n] = b[n] - temp * a[n - 2];
    f[n] = f[n] - temp * f[n - 2];
    /* 选主元 */
    if( fabs(a[n]) > fabs(b[n - 1]) )
    {
        temp = a[n]; a[n] = b[n - 1]; b[n - 1] = temp;
        temp = b[n]; b[n] = c[n - 1]; c[n - 1] = temp;
        temp = f[n]; f[n] = f[n - 1]; f[n - 1] = temp;
    }
    /* 最后一次消元 */
    temp = a[n] / b[n-1];
    a[n] = 0;
    b[n] = b[n] - temp * c[n - 1];
    f[n] = f[n] - temp * f[n - 1];
    
    /* 回代求解 m[] */
    m[n] = f[n] / b[n];
    m[n - 1] = (f[n - 1] - c[n - 1] * m[n]) / b[n-1];
    for(i = n - 2; i >= 0; i--)
    {
        m[i] = ( f[i] - (m[i + 2] * a[i] + m[i + 1] * c[i]) ) / b[i];
    }
    /************
    ** 放置参数
    ************/
    for(i = 0; i < n; i++)
    {
        coe[i].a3 = (m[i + 1] - m[i]) / (6 * h[i]);
        coe[i].b2 = m[i] / 2;
        coe[i].c1 = d[i] - (h[i] / 6) * (2 * m[i] + m[i + 1]);
        coe[i].d0 = y[i];
    }
    return n + 1;
}

/***********************************IMF********************************************/
//三次样条插值函数的表达式；
float func(float a,float b,float c,float d,float x,float y)
{
	float z = a * pow((x - y),3) + b * pow((x - y),2) + c * (x - y) + d;
	return z;
}
int imf(float x[N],float y[N],int b[N],float h[N]) //  输入：x，y，b； 输出：是否继续的条件；产生：imf分量；
{
	int i,j;
	int n = 0;
	int flag;
	int count = 0;
	point p_max;
	point p_min;
    
	for(i=0; i<N; i++)
	{
		h[i] = y[i];
	}
	/***************
	**因为是采集一定的点之后再做EMD，而且每次做完EMD之后就要进行FFT来确定哪些分量是跟呼吸或者心跳有用的频率分量，所以采样的点数和频率要和FFT
	**的相同。所以要采集512个点。但是此程序是测试程序，先将点的数目减少。
	**第一步：模拟产生原始信号的横纵坐标；
	**因为一次迭代不一定就求出了最高频，所以要反复迭代直到最后的平均包络为零；
	****************/
	do
	{
		flag = 0; //设置标志位，控制循环的条件；
	    //第二步：找极大值点，对极大值点进行包络，求出每两个极大值区间的包络函数；
		n = extr_max(h,b);   //极大值点的个数；

		if(n < 2)
		return 1; //判断极值点是否大于等于二,如果不满足条件，就直接退出；

		b[0] = 0;
		b[n+1] = N-1; 
		n = n+2; //这句话的意思是，要求整个陌纾由峡泛徒嵛驳牡悖时燃笾刀嗔礁龅恪�

		for(i=0; i<n; i++)
		{
			temp_x_max[i] = x[b[i]];
			temp_y_max[i] = h[b[i]];
		}
		p_max.xCoordinate = temp_x_max;
		p_max.yCoordinate = temp_y_max;
		p_max.f0 = 0;
		p_max.fn = 0;
		p_max.num = n;
		p_max.con = NOTaKNOT;
		p_max.coe = coe_max;
		spline(&p_max);
		j = n - 2;
		for(i=N-1; i>=0; i--)
		{
			if(x[i] >= x[b[j]])
			{
				m[i] = func(coe_max[j].a3,coe_max[j].b2,coe_max[j].c1,coe_max[j].d0,x[i],x[b[j]])/2;
			}
			else
			{
				j--;
				m[i] = func(coe_max[j].a3,coe_max[j].b2,coe_max[j].c1,coe_max[j].d0,x[i],x[b[j]])/2;
			}
		}

	   //第五剑赫壹≈档悖约≈档憬邪纾蟪雒苛礁黾≈登涞陌绾�
		n = extr_min(h,b);   //极小值点的个数；

		if(n < 2)
		return 1; //判断极值点是否大于等于二,如果不满足条件，就直接退出；

		b[0] = 0;
		b[n+1] = N-1;
		n = n+2; //这句话的意思是，要求整个函数的包络，要加上开头和结尾的点，故比极小值多两个点。
	
		for(i=0; i<n; i++)
		{
			temp_x_min[i] = x[b[i]];
			temp_y_min[i] = h[b[i]];
		}
		p_min.xCoordinate = temp_x_min;
		p_min.yCoordinate = temp_y_min;
		p_min.f0 = 0;
		p_min.fn = 0;
		p_min.num = n;
		p_min.con = NOTaKNOT;
		p_min.coe = coe_min;
		spline(&p_min);
	
	//求出整体的二分之一，即包括极大值点。
		j = n - 2; //j重新置数 这点很重要的！！
		for(i=N-1; i>=0; i--)
		{
			if(x[i] >= x[b[j]])
				m[i] += func(coe_min[j].a3,coe_min[j].b2,coe_min[j].c1,coe_min[j].d0,x[i],x[b[j]])/2;
			else
			{
				j--;
				m[i] += func(coe_min[j].a3,coe_min[j].b2,coe_min[j].c1,coe_min[j].d0,x[i],x[b[j]])/2;
			}
		}

		for(i=0; i<N; i++)
		{
				if(m[i] >= 0.1) // 不知道判定条件是不是过于严谨，有说用系数的，看情况；
				flag = 1;
		}

  	    //用原始的信号减去新产生的包络平均信号；
		for(i = 0; i< N; i++)
		{
			h[i] = h[i] - m[i]; 
		}
		//设置标志位，判断平均包络信号是否与x轴重合；
		count++;
 	if(count == 40)
 		flag = 0;
	}while(flag);  // 
	return 0;
}

void emd(float *x, float *y, float fc)//fc为对给进的初始信号进行fft之后求得的频谱最大值对应的频率
{
	
	int go_on;	
	int count1 = 0;
	int count2 = 0;
	int i = 0;
	int j = 0;
	float total = 0;
	float e_asp = 0;
	float e_hea = 0;
	comp data[N]; //申请FFT变换的静态内存；
    float h[N];  //存储y的值，保证y的值不被破坏；
	int b[N];
    float im[10][512];
    int num = 0;
	//FILE * fp;
	for(i=0; i<N; i++)
	{
		h[i] = y[i];
	}

	//存储IMF分量值；
   
	do
	{   
		go_on = imf(x,h,b,im[num]);
		for(j=0; j<N; j++)
		{
			h[j] = h[j] - im[num][j];
		}
		num++;
	}while(!go_on);
//	/*
//文件操作(1)
/*	fp = fopen("D:\\imf.txt","w");
	fprintf(fp,"###################\nThe number of imf is %d\n",count);
	//打印每个imf分量的数值；
	for(i=0; i<count; i++)
	{
		fprintf(fp,"\nIMF %d:\n",i);
		for(j=0; j<N; j++)
			fprintf(fp,"%f,",im[i][j]);
		fprintf(fp,"**************************\n");
	}
	fclose(fp);*/
//文件操作结束(1)
// */
	//以上就是分解出count个imf分量
	//*****************************************************************************************

	//以下要对分解出的每个imf分量进行FFT，然后对能量进行积分，求出对呼吸或者心跳有用的频率分量进行叠加；
	//初始值为零；
	for(i=0;i<N;i++)
	{
		asp[i].re = 0;
		asp[i].im = 0; 
		hea[i].re = 0;
		hea[i].im = 0;
	}
	for(i=0; i<num; i++)
	{
		for(j=0; j<N; j++)
		{
			data[j].re = im[i][j];
			data[j].im = 0;
		}
		//FFT;
		fft(data);
		//每个imf的频谱计算之前，要把频谱能量值清零；
		total = 0;
		e_asp = 0;
		e_hea = 0;
		//求出每个imf中呼吸和心跳能量的百分比，来重构呼吸和心跳信号；
		for(j=0; j<N/2; j++)   //频谱近似是对称的，所以取前一半，就是频率小于12.8Hz的；
		{
			total += pow(data[j].re, 2) ; //呼吸区间；

			if((float)freq*j/(N-1) >= (fc - 0.2) && (float)freq*j/(N-1) <= (fc + 0.2))
				e_asp += pow(data[j].re, 2) ;

			else if((float)freq*j/(N-1) >= (fc + 0.5) && (float)freq*j/(N-1) <= 3) //心跳区间； //应该是1和3，暂时改动，仿真用；
				e_hea += pow(data[j].re, 2) ;
		}
		if(e_asp/total >= 0.5)
		{
			count1++;
			for(j=0; j<N; j++)
			{
				asp[j].re += im[i][j];
			}
		}
		if(e_hea/total >= 0.5)
		{
			count2++;
			for(j=0; j<N; j++)
			{
				hea[j].re += im[i][j];
			}
		}
	}
//	/*
//文件操作(2)；
	/* fp=fopen("D:\\asp_hea.txt","w");
	 fprintf(fp,"**************************************************\n");
	 fprintf(fp,"count = %d\ncount1 = %d\ncount2 = %d\n",count,count1,count2);
	 fprintf(fp,"The aspiration value is:\n");
	 for(j=0; j<N; j++)
	 {
		fprintf(fp,"%f,",asp[j]);
	 }
	 fprintf(fp,"\n\nThe heartbeat value is:\n");
	 for(j=0; j<N; j++)
	 {
		fprintf(fp,"%f,",hea[j]);
	 }
	 fclose(fp);*/

}
//求呼吸率和心率的最大值并返回其对应的频率；

float max_val(comp *biosig)
{
	int i,max;
	max = 1;
	for(i=2; i<N/2; i++)
	{
		if(pow(biosig[max].re, 2) + pow(biosig[max].im, 2) < pow(biosig[i].re, 2) + pow(biosig[i].im, 2))
			max = i;
	}
	return (float)max*freq/N;
}

float g_x[N];


void emd_init()
{
	int i;
	for(i=0;i<N;i++)
	{
	  g_x[i]= (float)i* peri/(N-1);
	}	
}
//计算呼吸率和心率的函数
void calculate(float* voltage, calc_result* result)
{
	//FILE *fp;
	int i;
	float fcc;
    
	for(i=0; i<N; i++)  // h[i]获取y的值，来进行第一次的fft；
	{
		ht[i].re = voltage[i];
		ht[i].im = 0;
	}
//先对初始信号进行fft，求出呼吸率的估计值；
	fft(ht);
	fcc = max_val(ht);
  	log(INFO, "%f\n",fcc);
	emd(g_x,voltage,fcc);
	
	for (i = 0; i < N; i++) {
		result->raw[i] = voltage[i];
		result->asp[i] = asp[i].re;
		result->hea[i] = hea[i].re;	
	}
	
	fft(asp);
	fft(hea);
//文件写入(3)
//	/*
/*	fp=fopen("D:\\spectrum.txt","w");

	fprintf(fp,"The original value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",ht[i].re,ht[i].im);
	}

	fprintf(fp,"\n\nThe aspiration value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",asp[i].re,asp[i].im);
	}
	fprintf(fp,"\n\nThe heartbeat value after fft is:\n");
	for(i=0;i<N;i++)
	{
		fprintf(fp,"%f+%fi,",hea[i].re,hea[i].im);
	}
	fclose(fp);
//文件写入结束(3)
// */
	breath=max_val(asp);
	heart=max_val(hea);
		
	log(INFO, "The aspiration_rate is %fHz\n",breath);
	log(INFO, "The heartbeat_rate is %fHz\n",heart);
	
	result->asp_rate = breath;
	result->hea_rate = heart;
}






















