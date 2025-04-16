先配置测试环境

```sh
# 安装预编译的GTest和GMock
sudo apt update
sudo apt install -y libgtest-dev libgmock-dev

# 验证安装
ls /usr/include/gtest/      # 头文件位置
ls /usr/lib/x86_64-linux-gnu/libgtest*.a  # 库文件位置
```

添加好单元测试之后直接运行
`./tests/MyServerTests`