package com.SoyBeanMilk.RgbLedControl;

import androidx.appcompat.app.AppCompatActivity;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import com.SoyBeanMilk.RgbLedControl.widget.Blur.BlurExpandLayout;
import com.SoyBeanMilk.RgbLedControl.widget.ColorPicker.LineColorPicker;
import com.SoyBeanMilk.RgbLedControl.widget.ColorPicker.Palette;
import com.SoyBeanMilk.RgbLedControl.widget.SwitchBtn.SwitchButton;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    SwitchButton Link;
    BlurExpandLayout Fold;
    LineColorPicker colorPicker;
    TextView ShowColorRGB , ShowColorHEX;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            window.setStatusBarColor(Color.TRANSPARENT);
            window.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }

        setContentView(R.layout.activity_main);
        colorPicker = findViewById(R.id.picker);
        Link = findViewById(R.id.link);
        Fold = findViewById(R.id.fold);
        ShowColorRGB = findViewById(R.id.show_color_rgb);
        ShowColorHEX = findViewById(R.id.show_color_hex);

        Link.setOnCheckedChangeListener((view, isChecked) -> {
            if(isChecked && !Fold.getExpandable()){
                Toast.makeText(MainActivity.this , "Connect Successfully" , Toast.LENGTH_SHORT).show();
                Fold.ExpandLayout();
            }
            else{
                Fold.HideLayout();
            }
        });

        int[] colors = Palette.generateColors(Color.parseColor("#ff1500"),Color.parseColor("#b700ff") , 9);

        colorPicker.setColors(colors);
        colorPicker.setOnColorChangedListener(c -> {

            sendData(String.valueOf(c) + '\0');

            int[] RGB = Palette.toRGB(c);
            String content = String.format("R:%d  G:%d  B:%d" , RGB[0] , RGB[1] , RGB[2]);
            ShowColorRGB.setText(content);
            ShowColorRGB.setTextColor(Color.rgb(RGB[0] , RGB[1] , RGB[2]));

            String hexColor ="#" + Integer.toHexString(c).substring(2);
            ShowColorHEX.setTextColor(Color.parseColor(hexColor));
            ShowColorHEX.setText(hexColor);
        });

    }
    private void sendData(final String data) {
        // 创建一个新的线程，在后台执行发送数据的任务
        new Thread(() -> {
            try {
                // 连接到esp32的IP地址和端口，这里假设是192.168.4.1:80，你需要根据你的实际情况修改
                Socket socket = new Socket("192.168.4.1", 80);

                //Toast.makeText(MainActivity.this , "Connect Successfully!" , Toast.LENGTH_LONG).show();

                PrintWriter out = new PrintWriter(socket.getOutputStream()); // 创建一个输出流对象

                // 发送数据，这里使用println方法，会在数据后面自动添加换行符\n
                out.println(data);
                out.flush(); // 刷新输出流，确保数据被发送出去
                // 关闭连接
                out.close();
                socket.close();
            } catch (IOException e) {
                e.printStackTrace();
                try {
                    throw(e);
                } catch (IOException ex) {
                    throw new RuntimeException(ex);
                }
            }
        }).start();
    }

    private void receiveData(Socket socket) {
        try {
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream())); // 创建一个输入流对象
            // 接收数据，这里使用readLine方法，会读取一行以\n结尾的数据
            String receivedData = in.readLine();
            if (receivedData != null) {
                // 在文本视图上显示接收到的数据，注意需要在UI线程上执行
                runOnUiThread(() -> {
                    //TODO
                });
            }
            // 关闭输入流
            in.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
