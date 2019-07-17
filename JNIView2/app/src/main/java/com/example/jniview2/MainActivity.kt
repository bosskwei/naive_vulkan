package com.example.jniview2

import android.content.Context
import android.graphics.Canvas
import android.os.Bundle
import android.util.AttributeSet
import com.google.android.material.snackbar.Snackbar
import androidx.appcompat.app.AppCompatActivity
import android.view.Menu
import android.view.MenuItem
import android.widget.ImageView
import android.graphics.Bitmap
import android.widget.TextView
import kotlinx.android.synthetic.main.activity_main.*
import android.os.CountDownTimer
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.time.LocalTime


class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        setSupportActionBar(toolbar)

        fab.setOnClickListener { view ->
            Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                .setAction("Action", null).show()
        }

        val surfaceView: CustomSurfaceView = this.findViewById(R.id.surfaceView)
        val shader: ByteArray = this.assets.open("shader/comp_1.spv").readBytes()
        surfaceView.renderReady(shader)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        return when (item.itemId) {
            R.id.action_settings -> true
            else -> super.onOptionsItemSelected(item)
        }
    }
}

class CustomSurfaceView : SurfaceView, SurfaceHolder.Callback, Runnable {
    private var initialized: Boolean = false
    private var drawBitmap: Bitmap = Bitmap.createBitmap(1024, 1024, Bitmap.Config.ARGB_8888)
    private var drawCount: Long = 0
    private var drawTimeSec: Double = 0.0
    private external fun initNative(shader: ByteArray)
    private external fun renderNative(bitmap: Bitmap)

    constructor(context: Context) : super(context)
    constructor(context: Context, attrs: AttributeSet) : super(context, attrs)
    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

    init {
        System.loadLibrary("native-lib")
        this.holder.addCallback(this)
    }

    fun renderReady(shader: ByteArray) {
        this.initNative(shader)
        this.initialized = true

        val textView: TextView = (this.context as MainActivity).findViewById(R.id.textView)

        object : CountDownTimer(0xFFFFFFFF, 1000) {
            private var lastTickCount: Long = 0
            private var lastTickNanoSec: Long = LocalTime.now().toNanoOfDay()
            private var firstDrawNanoSec: Long = LocalTime.now().toNanoOfDay()

            override fun onTick(millisUntilFinished: Long) {
                val now = LocalTime.now().toNanoOfDay()
                val fps: Double =
                    (drawCount - this.lastTickCount).toDouble() / (1e-9 * (now - this.lastTickNanoSec + 1).toDouble())
                val loadSec: Double = drawTimeSec / (1e-9 * (now - this.firstDrawNanoSec + 1).toDouble())
                textView.text = ("FPS: %.2f, Load: %.2f".format(fps, loadSec))
                this.lastTickCount = drawCount
                this.lastTickNanoSec = now
            }

            override fun onFinish() {
            }
        }.start()
    }

    override fun surfaceCreated(holder: SurfaceHolder?) {
        Log.i("CustomSurfaceView", "surfaceCreated()")
        Thread(this).start()
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
        Log.i("CustomSurfaceView", "surfaceChanged()")
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
        Log.i("CustomSurfaceView", "surfaceDestroyed()")
    }

    override fun run() {
        Log.i("CustomSurfaceView", "run()")
        while (holder != null) {
            val canvas: Canvas = holder.lockCanvas() ?: return

            val before = LocalTime.now().toNanoOfDay()
            this.renderNative(this.drawBitmap)
            canvas.drawBitmap(this.drawBitmap, 0.0f, 0.0f, null)
            val after = LocalTime.now().toNanoOfDay()

            val diff = after - before
            this.drawCount += 1
            this.drawTimeSec += 1e-9 * diff.toDouble()

            holder.unlockCanvasAndPost(canvas)
        }
    }
}
