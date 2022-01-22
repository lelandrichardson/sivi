package sivi.pocket

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.material.CircularProgressIndicator
import androidx.compose.material.Icon
import androidx.compose.material.IconButton
import androidx.compose.material.Text
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Adjust
import androidx.compose.material.icons.filled.Clear
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.CacheDrawScope
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.consumeAllChanges
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.center
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.toOffset
import sivi.pocket.ui.theme.*
import kotlin.math.*


const val ASPECT_RATIO = 3f / 2f

private enum class EditMode {
  Rotation,
  Range
}

@Composable fun Rotator() {
  val scope = rememberCoroutineScope()
  var mode by remember { mutableStateOf(EditMode.Rotation) }
  val model = remember { RotatorModel(scope, FakeRotatorApi) }
  val dragMain = remember {
    DraggableAngle { model.goTo(it) }
  }

  val dragRange = remember {
    DraggableAngleRange(
      { model.updateMin(it) },
      { model.updateMax(it) },
    )
  }
  if (
    !model.isLoading &&
    !dragMain.isDragging &&
    model.currentDegrees != dragMain.current
  ) {
    SideEffect {
      dragMain.syncAngle(model.currentDegrees)
    }
  }
  if (!model.isLoading &&
    !dragRange.isDragging &&
    (model.maxDegrees != dragRange.currentMax || model.minDegrees != dragRange.currentMin)
  ) {
    SideEffect {
      dragRange.syncRange(model.minDegrees, model.maxDegrees)
    }
  }
  val dragModifiers = when (mode) {
    EditMode.Rotation -> dragMain.modifier
    EditMode.Range -> dragRange.modifier
  }
  Box(
    Modifier
      .fillMaxSize()
      .background(Brush.radialGradient(listOf(bgColorCenter, bgColorEdge))),
    contentAlignment = Alignment.Center
  ) {
    CompassLayout(
      Modifier
        .fillMaxWidth()
        .padding(10.dp)
        .then(dragModifiers),
      min = model.minDegrees,
      max = model.maxDegrees,
    ) {
      when (mode) {
        EditMode.Rotation -> {
          FOVIndicator(Modifier.padding(60.dp), model.currentDegrees, lightGrey, ASPECT_RATIO)
          FOVIndicator(Modifier.padding(60.dp), dragMain.current, darkRed, ASPECT_RATIO)
          if(!model.isLoading) {
            Text(
              "${dragMain.current.roundToInt()}°",
              color = Color.White,
              fontSize = 48.sp,
              textAlign = TextAlign.Center
            )
          }
        }
        EditMode.Range -> {
          RangeHandle(model.minDegrees, lightGrey, true)
          RangeHandle(model.maxDegrees, lightGrey, false)

          RangeHandle(dragRange.currentMin, darkRed, true)
          RangeHandle(dragRange.currentMax, darkRed, false)
          if(!model.isLoading) {
            Text(
              "${dragRange.currentMin.roundToInt()}° - ${dragRange.currentMax.roundToInt()}°",
              color = Color.White,
              fontSize = 48.sp,
              textAlign = TextAlign.Center
            )
          }
        }
      }
      if (model.isLoading) {
        CircularProgressIndicator(color = darkRed)
      }
    }
    Row(
      Modifier
        .align(Alignment.BottomCenter)
        .padding(bottom = 80.dp)
    ) {
      IconButton(onClick = {
        mode = when (mode) {
          EditMode.Range -> EditMode.Rotation
          EditMode.Rotation -> EditMode.Range
        }
      }) {
        Icon(Icons.Default.Adjust, contentDescription = "Adjust")
      }
      IconButton(onClick = {
        model.zero()
      }) {
        Icon(Icons.Default.Clear, contentDescription = "Clear")
      }
    }
  }
}

@Composable fun FOVIndicator(
  modifier: Modifier = Modifier,
  rotation: Double,
  color: Color,
  aspectRatio: Float,
) {
  Box(
    modifier
      .rotate(
        rotation
          .roundToInt()
          .toFloat()
      )
      .aspectRatio(aspectRatio)
      .border(1.dp, color)
      .drawBehind {
        drawLine(
          color,
          start = Offset(center.x, 0f),
          end = Offset(center.x, -center.y * 0.25f),
          strokeWidth = 1.dp.toPx()
        )
      }
  )
}

@Composable fun RangeHandle(
  degrees: Double,
  color: Color,
  start: Boolean,
) {
  Box(
    Modifier
      .fillMaxSize()
      .drawBehind {
        val theta = ((degrees - 90) * PI / 180).toFloat()
        val radius = size.width / 2f
        val inner = radius * 0.5f
        val len = radius * 0.125f
        val circleCenter = polarOffset(inner, theta)
        val direction = if (start) 1 else -1
        drawTick(color, theta, inner, radius)
        val a = polarOffset(inner, theta)
        val b = polarOffset(inner, theta + (direction * 10.0 / 180.0 * PI).toFloat())
        val c = polarOffset(inner + len, theta)
        val p = Path().apply {
          moveTo(a.x, a.y)
          lineTo(b.x, b.y)
          lineTo(c.x, c.y)
          lineTo(a.x, a.y)
          close()
        }
        drawPath(p, color)
//        drawLine(color, a, b)
//        drawLine(color, b, c)
//        drawLine(color, c, a)
        drawCircle(color, radius * 0.125f, center = circleCenter)
      }
  )
}

@Composable
fun CompassLayout(
  modifier: Modifier,
  min: Double,
  max: Double,
  content: @Composable () -> Unit
) {
  Box(
    modifier
      .aspectRatio(1f)
      .drawBehind {
        val radius = size.width / 2f
        val baseLength = radius * 0.08f
        val invert = max < min
        for (i in 0 until 360 step 3) {
          val color = when {
            invert && (i > max && i < min) -> lightGrey
            !invert && (i < min || i > max) -> lightGrey
            i % 30 == 0 -> darkRed
            else -> lightOrange
          }
          val scale = when {
            i % 90 == 0 -> 2f
            i % 30 == 0 -> 1.5f
            else -> 1f
          }
          val startRadius = radius - baseLength * scale
          val angle = i - 90 // -180 to 180
          val theta = angle * PI.toFloat() / 180f // radians
          drawTick(color, theta, startRadius, radius)
        }
        drawCircle(
          darkRed,
          radius,
          style = Stroke(TickWidth)
        )
      },
    contentAlignment = Alignment.Center
  ) {
    content()
  }
}

const val TickWidth = 9f
const val Epsilon = 9f

fun DrawScope.drawTick(
  color: Color,
  theta: Float,
  startRadius: Float,
  endRadius: Float
) {
  drawLine(
    color,
    polarOffset(startRadius, theta),
    polarOffset(endRadius, theta),
    TickWidth,
    StrokeCap.Round,
  )
}

fun DrawScope.polarOffset(r: Float, theta: Float): Offset {
  return center + Offset(
    cos(theta) * r,
    sin(theta) * r
  )
}

inline fun Modifier.iff(cond: Boolean, mod: () -> Modifier): Modifier {
  return this.then(if (cond) mod() else Modifier.padding(0.dp))
}

class DraggableAngle(private val onChange: (Double) -> Unit, ) {
  var current: Double by mutableStateOf(0.0)
    private set
  private var origin by mutableStateOf(Offset.Zero)
  private var position by mutableStateOf<Offset?>(null)
  var isDragging by mutableStateOf(false)
    private set

  fun syncAngle(degrees: Double) {
    current = degrees
  }

  val modifier = Modifier
    .onSizeChanged { origin = it.center.toOffset() }
    .pointerInput(this) {
      detectDragGestures(
        onDragStart = { offset ->
          val pos = offset - origin
          val angle = pos.theta + 90
          println("onDragStart: angle: $angle, current: $current")
          if (degDiff(angle, current) < 20) {
            println("onDragStart: dragging!")
            isDragging = true
            position = offset - origin
          }
        },
        onDragEnd = {
          println("onDragEnd")
          if (isDragging) {
            isDragging = false
            onChange(current)
          }
        },
        onDragCancel = {
          if (isDragging) {
            isDragging = false
            onChange(current)
          }
        },
        onDrag = { change, delta ->
          if (!isDragging) return@detectDragGestures
          val prev = position
          position = if (prev != null) {
            change.consumeAllChanges()
            val prevTheta = prev.theta
            val next = prev + delta
            val deltaTheta = next.theta - prevTheta
            current = deg(current + deltaTheta)
            next
          } else {
            delta
          }
        }
      )
    }
}

class DraggableAngleRange(
  private val onMinChange: (Double) -> Unit,
  private val onMaxChange: (Double) -> Unit,
) {
  var currentMin: Double by mutableStateOf(0.0)
    private set
  var currentMax: Double by mutableStateOf(0.0)
    private set
  private var origin by mutableStateOf(Offset.Zero)
  private var position by mutableStateOf<Offset?>(null)
  var dragDirection by mutableStateOf(0)
    private set
  val isDragging: Boolean get() = dragDirection != 0

  fun syncRange(min: Double, max: Double) {
    currentMin = min
    currentMax = max
  }

  val modifier = Modifier
    .onSizeChanged { origin = it.center.toOffset() }
    .pointerInput(this) {
      detectDragGestures(
        onDragStart = { offset ->
          val pos = offset - origin
          val angle = pos.theta + 90
          val minDiff = degDiff(angle, currentMin)
          val maxDiff = degDiff(angle, currentMax)
          if (minDiff < maxDiff && minDiff < 20) {
            dragDirection = -1
            position = offset - origin
          } else if (maxDiff < 20) {
            dragDirection = 1
            position = offset - origin
          }
        },
        onDragEnd = {
          if (dragDirection == -1) {
            onMinChange(currentMin)
          } else if (dragDirection == 1) {
            onMaxChange(currentMax)
          }
          dragDirection = 0
        },
        onDragCancel = {
          if (dragDirection == -1) {
            onMinChange(currentMin)
          } else if (dragDirection == 1) {
            onMaxChange(currentMax)
          }
          dragDirection = 0
        },
        onDrag = { change, delta ->
          if (!isDragging) return@detectDragGestures
          val prev = position
          position = if (prev != null) {
            change.consumeAllChanges()
            val dragVar = if (dragDirection == 1) ::currentMax else ::currentMin
            val prevTheta = prev.theta
            val next = prev + delta
            val deltaTheta = next.theta - prevTheta
            dragVar.set(deg(dragVar.get() + deltaTheta))
            next
          } else {
            delta
          }
        }
      )
    }
}

fun degDiff(a: Double, b: Double): Double {
  val delta = deg(a - b)
  return min(delta, abs(delta-360))
}

val Offset.theta: Double get() = atan2(y.toDouble(), x.toDouble()) * 180.0 / PI

val CacheDrawScope.center: Offset get() = Offset(size.width / 2, size.height / 2)