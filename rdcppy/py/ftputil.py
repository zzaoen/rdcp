import ftplib
import os
from tkinter import *
from fileutil import *
import time

host = '192.168.0.100'
username = 'lab1'
password = '123'
remote_path = 'files'
is_login = True

# f = ftplib.FTP(host)
# f.login(username, password)
# f.cwd(remote_path)


def test_remote_dir(f, remote_path):
    print(f.pwd(), remote_path)
    try:
        f.cwd(remote_path)
        print(f.pwd())
        return 1
    except ftplib.error_perm:
        try:
            f.mkd(remote_path)
            return 1
        except ftplib.error_perm:
            print('请确认是否有权限新建文件夹')
            return 0
    # finally:
    #     f.quit()
    #     f.close()

def ftp_upload_file(file_path, remote_dir, f, lb):
    print(file_path, remote_dir)


    bufsize = 1024
    fp = open(file_path, 'rb')
    _, file_name = os.path.split(file_path)
    remote_file_name = os.path.join(remote_dir, file_name)

    if get_file_size(file_path) < 5:
        try:
            f.storbinary('STOR ' + remote_file_name, fp, bufsize)
            lb.insert(END, file_path)
            lb.insert(END, "所有文件发送完毕！")
        except ftplib.error_perm:
            pass
    else:
        fp_local = open('/tmp/rdma_files_local', 'w+')
        fp_remote = open('/tmp/rdma_files_remote', 'w+')

        fp_local.write(file_path)
        fp_local.write('\n')
        fp_remote.write(remote_file_name)
        fp_remote.write('\n')
        fp_local.close()
        fp_remote.close()

        lb.insert(END, "rdma正在发送大文件，请稍后...")
        os.system('./rdcp-script2.sh')
        lb.insert(END, file_path)
        lb.insert(END, "所有文件发送完毕！")



def ftp_upload_file_util(local_path, remote_path, f):
    bufsize = 1024
    fp = open(local_path, 'rb')
    try:
        f.storbinary('STOR ' + remote_path, fp, bufsize)
    except ftplib.error_perm:
        pass

def ftp_upload_dir(dir_path, remote_dir, f, lb, root):
    time_start = time.time()
    total_size = [0]


    local_rdma_list = list()
    local_ethe_list = list()
    local_dir_list = list()
    process_dir(dir_path, local_rdma_list, local_ethe_list, local_dir_list, total_size)

    remote_rdma_list = list()
    remote_ethe_list = list()
    remote_dir_list = list()

#把local的文件路径转成remote的
#/home/lab2/files/dir/file to /home/lab1/files/dir/file
    change_local_to_remote(dir_path, remote_dir, local_dir_list, remote_dir_list,)
    change_local_to_remote(dir_path, remote_dir, local_ethe_list, remote_ethe_list)
    change_local_to_remote(dir_path, remote_dir, local_rdma_list, remote_rdma_list)


# 先处理文件夹，先把文件夹传过去
    if len(local_dir_list) > 0:
        for i in remote_dir_list:
            try:
                f.mkd(i)
            except ftplib.error_perm:
                pass
            finally:
    #!!!!!!!!!!!!!!!!!!!!!!!!!!!
                lb.insert(END, i)
                pass
    root.update()
# 用以太网传小文件
    if len(local_ethe_list) > 0:
        for i in range(len(local_ethe_list)):
            print(local_ethe_list[i], remote_ethe_list[i])
            ftp_upload_file_util(local_ethe_list[i], remote_ethe_list[i], f)
            lb.insert(END, local_ethe_list[i])
    root.update()
    if len(local_rdma_list) > 0:
        fp_local = open('/tmp/rdma_files_local', 'w+')
        fp_remote = open('/tmp/rdma_files_remote', 'w+')
        for i in range(len(local_rdma_list)):
            fp_local.write(local_rdma_list[i])
            fp_local.write('\n')
            fp_remote.write(remote_rdma_list[i])
            fp_remote.write('\n')
        fp_local.close()
        fp_remote.close()

        lb.insert(END, "rdma正在发送大文件，请稍后...")
        os.system('./rdcp-script2.sh')
        for i in local_rdma_list:
            lb.insert(END, i)
        lb.insert(END, "所有文件发送完毕！")

    time_end = time.time()
    tsize = round(total_size[0]/(1024*1024), 2)
    ttime = round(time_end-time_start, 2)
    # lb.insert(END, '总大小: ' + str(total_size[0]) + 'MB' + '用时: ' + str(time_end-time_start))
    lb.insert(END, '总大小: {} MB, 用时: {} s'.format(tsize, ttime))

    # print('rdma')
    # for i in remote_rdma_list:
    #     print(i)
    #
    # print('ethernet')
    # for i in remote_ethe_list:
    #     print(i)
    #
    # print('dir')
    # for i in remote_dir_list:
    #     print(i)


if __name__ == '__main__':
    f = ftplib.FTP(host)
    f.login(username, password)
    # file_path = '/home/lab2/PycharmProjects/rdcppy/rdcp-script.sh'
    # remote_dir = '/home/lab1/files/'
    # ftp_upload_file(file_path, remote_dir, f)


    dir_path = '/home/lab2/files'
    remote_dir = '/home/lab1/files'
    ftp_upload_dir(dir_path, remote_dir, f, None)

    f.close()