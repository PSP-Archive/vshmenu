
#include "common.h"


extern char full_buff[192];


#define SECTOR_SIZE	0x800

const char umd_none[] = "NONE";

u8	cachechanged = 0;
static ISO_cache cache[32];
u8 referenced[32];
int video_cnt=0;
int current_video_cnt=-1;

char *GetVideoName(void)
{

	if( current_video_cnt >=0 && current_video_cnt<=31)
		return cache[current_video_cnt].isofile;

	return (char *)umd_none;
}

int GetVideoType(void)
{

	if( current_video_cnt >=0 && current_video_cnt <=31)
		return cache[current_video_cnt].type;

	return 0;
}
void change_umd_video(int dir )
{
	int sel = current_video_cnt;

	if (video_cnt == 0)
	{
		current_video_cnt = -1;
		return;//-1
	}

	if(dir == 0)
	{
		current_video_cnt = -1;
		return;
	}
	
	do
	{
		sel = limit(sel+dir, -1 , 32 - 1);

		if(sel == -1)
			break;

	}while(referenced[sel] == 0);

	current_video_cnt = sel;

	return;
}


int ReadCache()
{
	SceUID fd;
	int i;

	scePaf_memset(cache, 0, sizeof(ISO_cache)*32);
	scePaf_memset(referenced, 0, sizeof(referenced));

	for (i = 0; i < 0x10; i++)
	{
		fd = sceIoOpen("ms0:/PSP/SYSTEM/isocachev.bin", PSP_O_RDONLY, 0);

		if (fd >= 0)
			break;
	}

	if (fd < 0)
		return -1;

	sceIoRead(fd, cache, sizeof(ISO_cache)*32);
	sceIoClose(fd);

	return 0;
}

int SaveCache()
{
	SceUID fd;
	int i;

	//clean
	for (i = 0; i < 32; i++)
	{
		if (cache[i].isofile[0] != 0 && !referenced[i])
		{
			cachechanged = 1;
			scePaf_memset(&cache[i], 0, sizeof(ISO_cache));
		}
	}

	if (cachechanged)
	{

		cachechanged = 0;

		for (i = 0; i < 0x10; i++)
		{
			fd = sceIoOpen("ms0:/PSP/SYSTEM/isocachev.bin", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0511);

			if (fd >= 0)
				break;
		}

		if (fd < 0)
			return -1;

		sceIoWrite(fd, cache, sizeof(ISO_cache)*32);
		sceIoClose(fd);
		
	}

	return 0;
}

int IsCached(char *isofile, ScePspDateTime *mtime ,int *iso_index)//, VirtualPbp *res
{
	int i;
	int len;

	for (i = 0; i < 32; i++)
	{
		if (cache[i].isofile[0] != 0)
		{

			len = scePaf_strlen(isofile );

			if (scePaf_memcmp(cache[i].isofile, isofile , len+1) == 0)//
			{
				if (scePaf_memcmp(mtime, &cache[i].mtime, sizeof(ScePspDateTime)) == 0)
				{
					//memcpy(res, &cache[i], sizeof(ISO_cache));
					referenced[i] = 1;
					iso_index[0]= i;
					return 1;
				}
			}
		}
	}

	return 0;
}

int Cache(ISO_cache *pbp, int *iso_index)
{
	int i;

	for (i = 0; i < 32; i++)
	{

		if (cache[i].isofile[0] == 0)
		{
			referenced[i] = 1;
			memcpy(&cache[i], pbp, sizeof(ISO_cache));
			cachechanged = 1;
			iso_index[0]= i;
			return 1;
		}
	}

	return 0;
}

int MakeCache(const char *path , ScePspDateTime *m_time ,ISO_cache *pbp ,int *iso_index)
{
	int fd;
//	int type;
	int i;
	int cnt;

	scePaf_memset(pbp, 0, sizeof(ISO_cache));

	scePaf_strcpy( pbp->isofile , path);

	scePaf_snprintf( full_buff , 192 , "ms0:/ISO/VIDEO/%s" , path);
	//scePaf_sprintf( full_buff , "ms0:/ISO/VIDEO/%s" , path);
		
	fd =sceIoOpen( full_buff , 1, 0);
	
	if(fd >=0)		
	{			
		sceIoLseek(fd, 0x8000, PSP_SEEK_SET);
	
		if( sceIoRead(fd , full_buff , 144) == 144)		
		{
			if (scePaf_B6ADE52D_memcmp( &full_buff[1] , "CD001" , 5) ==0)	
			{

				//my_print(logfile , "iso is CD0001\n");

				int *offset = (int *)full_buff;
				sceIoLseek(fd, SECTOR_SIZE * offset[140/4], PSP_SEEK_SET);
				sceIoRead(fd , full_buff , 192);
				sceIoClose(fd);

				char *inter=(char *)full_buff;
				

				for(i=0;i<4;i++)
				{		

					if(inter[0]==9 || inter[0]==8)
					{
						if(scePaf_memcmp( &inter[8] , "UMD_VIDEO", 9) == 0)
						{
							pbp->type |= 0x20;
						}
						else if(scePaf_memcmp(&inter[8] , "UMD_AUDIO", 9)==0)
						{
							pbp->type |= 0x40;
						}
						/*
						else if(scePaf_memcmp(&inter[8] , "PSP_GAME", 8)==0)
						{
							pbp->type |= 0x10;
						}
						*/
						
					}

					cnt = (inter[0]&1) + inter[0] + 8;
					inter+=cnt;
				}

				if(pbp->type)//type
				{
					scePaf_memcpy( &(pbp->mtime) , m_time ,sizeof(ScePspDateTime));
					return Cache(pbp , iso_index);
				}

				return 0;
			
			}

		}

	}

	sceIoClose(fd);
	return -1;
}



int init_videoCache(const char *iso_name)
{
	SceIoDirent sp;//[352]
	ISO_cache cache_s;
	//int res;
	int len;
	int dir=sceIoDopen("ms0:/ISO/VIDEO");

	if(dir<0)
		return dir;

	ReadCache();
	
	scePaf_memset( &sp , 0 , sizeof(SceIoDirent));

	int iso_index = 0;
	video_cnt = 0;

	while(sceIoDread(dir , &sp) > 0)
	{

		if( FIO_S_ISDIR(sp.d_stat.st_mode))
			continue;

		if(IsCached( sp.d_name, &(sp.d_stat.st_mtime) , &iso_index) )// , &cache_s 
		{			
			//my_print(logfile , "isCache\n");
			video_cnt ++;
		}
		else if (MakeCache( sp.d_name ,  &(sp.d_stat.st_mtime) , &cache_s , &iso_index) > 0 )
		{	
			//my_print(logfile , "Makecache\n");
			video_cnt ++;
		}else
		{
			//my_print(logfile , "else continue\n");
			continue;
		}

		if(iso_name)
		{
			if(iso_name[0])
			{		
				len = scePaf_strlen( iso_name );

				if(scePaf_memcmp( sp.d_name , iso_name , len+1) == 0)
					current_video_cnt = iso_index;
			}
		}
	}

	sceIoDclose( dir );

	return SaveCache();
}
