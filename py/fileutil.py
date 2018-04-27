import os


def get_file_size(file_path, total_size):
    fsize = os.path.getsize(file_path)
    total_size[0] += fsize
    fsize = fsize / (1024*1024)
    return int(fsize)

def process_dir(dir_path, local_rdma_list, local_ethe_list, local_dir_list, total_size):


    for (root, dirs, files) in os.walk(dir_path):
        for filename in files:
            temp_file_local = os.path.join(root, filename)
            # temp_file_remote = os.path.join(remote_dir, filename)
            if get_file_size(temp_file_local, total_size) < 5:
                local_ethe_list.append(temp_file_local)
                # remote_ethe_list.append(temp_file_remote)
            else:
                local_rdma_list.append(temp_file_local)
                # remote_rdma_list.append(temp_file_remote)
        for dirc in dirs:
            local_dir_list.append(os.path.join(root, dirc))
            # remote_dir_list.append(os.path.join(remote_dir, dirc))


def change_local_to_remote(dir_path, remote_dir, local_paths, remote_paths):
    local_dir_length = len(dir_path)+1
    for i in local_paths:
        # _, file_name = os.path.split(i)
        sub_root_file_name = i[local_dir_length:]
        remote_paths.append(os.path.join(remote_dir, sub_root_file_name))


if __name__ == '__main__':
    # file_path = '/home/lab2/PycharmProjects/rdcppy/rdcp-script.sh'
    file_path = '/home/lab2/files/linux-4.4.4.tar.xz'
    # print(get_file_size(file_path))

    dir_path = '/home/lab2/files'
    remote_dir = '/home/lab1/files'
    local_rdma_list = list()
    local_ethe_list = list()
    local_dir_list = list()
    remote_rdma_list = list()
    remote_ethe_list = list()
    remote_dir_list = list()
    process_dir(dir_path, remote_dir, local_rdma_list, local_ethe_list, local_dir_list)

    print(local_rdma_list)
    print(local_ethe_list)
    print(local_dir_list)

    change_local_to_remote(dir_path, remote_dir, local_dir_list, remote_dir_list)
    print(remote_dir_list)
    # print('rdma')
    # for i in local_rdma_list:
    #     print(i)
    #
    # print('ethernet')
    # for i in local_ethe_list:
    #     print(i)
    #
    # print('dir')
    # for i in local_dir_list:
    #     print(i)