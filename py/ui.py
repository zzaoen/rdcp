from tkinter import *
import tkinter.filedialog
from tkinter.filedialog import askopenfilename
from tkinter.filedialog import askdirectory

from ftputil import *
import ftplib

root = Tk()
# root.geometry('50x200')
# Label(root, text="作品:").grid(row=0, column=0)
# Label(root, text="作者:").grid(row=1, column=0)
#
#
# e1 = Entry(root)
# e2 = Entry(root)
# e1.grid(row=0, column=1, pady=5)
# e2.grid(row=1, column=1, pady=5)
#
# def show():
#     print("作品: 《%s》" % '123')
#     print("作者: 《%s》" % '123')


default_dir = '/home/lab2/files'
filepath = StringVar()
dirpath = StringVar()

remotedir = StringVar()

def select_file_path():
    path_ = askopenfilename(title = '选择文件', initialdir = default_dir)
    filepath.set(path_)

def select_dir_path():
    path_ = askdirectory(title = '选择文件夹', initialdir = default_dir)
    dirpath.set(path_)

def exit():
    f.quit()
    f.close()
    root.quit()


# f = ftplib.FTP(host)
f = ftplib.FTP(host)
f.login(username, password)
def send_file():
    file_path = filepath.get()
    remote_dir = remotedir.get()
    if file_path == '':
        print('发送的文件的路径有误！')
    if remote_dir == '':
        print('必须指定目标文件夹路径')
    if file_path != '' and remote_dir != '':
        print('file name is {} and targetpath is {}'.format(file_path, remote_dir))

    # if ftputil.is_login == False:
    #     pass
    # else:
    #     f = ftplib.FTP(host)
    #     f.login(username, password)
    #     ftputil.is_login = True


    test_remote_dir(f, remote_dir)
    ftp_upload_file(file_path, remote_dir, f, lb)





def send_dir():
    dir_path = dirpath.get()
    remote_dir = remotedir.get()
    if dir_path == '':
        print('发送的文件夹的路径有误！')
    if remote_dir == '':
        print('必须指定目标文件夹路径')
    if dir_path != '' and remote_dir != '':
        print('file name is {} and targetpath is {}'.format(dir_path, remote_dir))

    test_remote_dir(f, remote_dir)
    ftp_upload_dir(dir_path, remote_dir, f, lb, root)


label1 = Label(root, text="目标文件夹", height = 2, width=10)
target_dir_entry = Entry(root, textvariable = remotedir, width=40)
label1.grid(row=0, column=0, sticky=W,padx=10,pady=5)
target_dir_entry.grid(row = 0, column = 1, columnspan=2, sticky=W+E+N+S)

# 选择目标文件夹上留下的空间
Label(root, text="", height = 2, width=10).grid(row = 1, column = 1, sticky=N+S)


btn1 = Button(root, text = "选择文件", command = select_file_path, height = 2, width=10)
entry1 = Entry(root, textvariable = filepath, width=40)
btn2 = Button(root, text = "传输", command = send_file, height = 2, width=10)
# label1.grid(row = 0, column = 0, sticky=N+S)
btn1.grid(row = 2, column =0, sticky=N+S)
entry1.grid(row = 2, column = 1, sticky=N+S)
btn2.grid(row = 2, column =2, sticky=N+S)

# label2 = Label(root,text = "文件夹", height = 2, width=10)
btn3 = Button(root, text = "选择文件夹", command = select_dir_path, height = 2, width=10)
entry2 = Entry(root, textvariable = dirpath, width=40)
btn4 = Button(root, text = "传输", command = send_dir, height = 2, width=10)
btn3.grid(row = 3, column = 0, sticky=N+S)
entry2.grid(row = 3, column = 1, sticky=N+S)
btn4.grid(row = 3, column = 2, sticky=N+S)





Label(root, text="", height = 2, width=10).grid(row = 5, column = 1, sticky=N+S)


lb = Listbox(root, exportselection=False, height=10, width=40)

scr1 = Scrollbar(root)
lb.configure(yscrollcommand = scr1.set)
scr1['command']=lb.yview
scr1.grid(row=6,column=0)

scr2 = Scrollbar(root,orient='horizontal')
lb.configure(xscrollcommand = scr2.set)
scr2['command']=lb.xview
scr2.grid(row=7,column=1)



scr3 = Scrollbar(root)
lb.configure(yscrollcommand = scr3.set)
scr3['command']=lb.yview
scr3.grid(row=6,column=2)

lb.grid(row=6,column=1)

Button(root, text = "exit", command = exit, height = 2, width=10).grid(row=8)

root.mainloop()
