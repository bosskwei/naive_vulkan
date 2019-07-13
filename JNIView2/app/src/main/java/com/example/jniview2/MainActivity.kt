package com.example.jniview2

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.os.Bundle
import android.util.AttributeSet
import com.google.android.material.snackbar.Snackbar
import androidx.appcompat.app.AppCompatActivity
import android.view.Menu
import android.view.MenuItem
import android.widget.ImageView
import android.graphics.Bitmap
import kotlinx.android.synthetic.main.activity_main.*
import kotlin.math.min


class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        setSupportActionBar(toolbar)

        fab.setOnClickListener { view ->
            Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                .setAction("Action", null).show()
        }

        val shader: ByteArray = this.assets.open("shader/comp_1.spv").readBytes()
        val imageView: CustomImageView = this.findViewById(R.id.imageView)
        imageView.renderReady(shader)
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

class CustomImageView : ImageView {
    private var initialized: Boolean = false
    private var drawBitmap: Bitmap = Bitmap.createBitmap(1024, 1024, Bitmap.Config.ARGB_8888)
    private external fun initNative(shader: ByteArray)
    private external fun renderNative(bitmap: Bitmap)

    init {
        System.loadLibrary("native-lib")
    }

    constructor(context: Context) : super(context)
    constructor(context: Context, attrs: AttributeSet) : super(context, attrs)
    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(context, attrs, defStyleAttr)

    fun renderReady(shader: ByteArray) {
        this.initNative(shader)
        this.initialized = true
    }

    override fun onDraw(canvas: Canvas) {
        if (!this.initialized) {
            return
        }

        this.renderNative(this.drawBitmap)
        canvas.drawBitmap(this.drawBitmap, 0.0f, 0.0f, null)
        this.invalidate()
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        // drawBitmap = Bitmap.createBitmap(min(h, w), min(h, w), Bitmap.Config.ARGB_8888)
        // super.onSizeChanged(w, h, oldw, oldh)
    }
}