package sivi.pocket

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.random.Random

interface RotatorApi {
  suspend fun position(): Int
  suspend fun isMoving(): Boolean
  suspend fun min(): Int
  suspend fun max(): Int
  suspend fun stop()
  suspend fun save()
  suspend fun restart()
  suspend fun goTo(pos: Int)
  suspend fun setMin(pos: Int)
  suspend fun setMax(pos: Int)
}

object FakeRotatorApi: RotatorApi {
  private suspend fun delayRandom() {
    delay(800 + Random.nextLong(400))
  }
  private var position: Int = 0
  private var min: Int = -1600
  private var max: Int = 1600
  override suspend fun position(): Int {
    delayRandom()
    return position
  }
  override suspend fun isMoving(): Boolean {
    // TODO: implement
    return false
  }
  override suspend fun min(): Int {
    delayRandom()
    return min
  }
  override suspend fun max(): Int {
    delayRandom()
    return max
  }
  override suspend fun stop() {
    delayRandom()
  }
  override suspend fun save() {
    delayRandom()
  }
  override suspend fun restart() {
    delayRandom()
  }
  override suspend fun goTo(pos: Int) {
    delayRandom()
    position = pos
  }
  override suspend fun setMin(pos: Int) {
    delayRandom()
    min = pos
  }
  override suspend fun setMax(pos: Int) {
    delayRandom()
    max = pos
  }
}


class RotatorModel(
  private val scope: CoroutineScope,
  private val api: RotatorApi
) {
  private var sequenceCount by mutableStateOf(0)
  val degreesPerStep = 0.1
  val stepsPerRotation: Int get() = (360 / degreesPerStep).roundToInt()
  var zeroStep by mutableStateOf(0)
  var currentStep: Int by mutableStateOf(0)
    private set
  var minStep by mutableStateOf(degreeToStep(0.0))
    private set
  var maxStep by mutableStateOf(degreeToStep(360.0))
    private set

  init {
    launch {
      currentStep = api.position()
      minStep = api.min()
      maxStep = api.max()
    }
  }

  val isLoading: Boolean get() = sequenceCount > 0
  val currentDegrees: Double
    get() = stepToDegree(currentStep)
  val minDegrees: Double
    get() = stepToDegree(minStep)
  val maxDegrees: Double
    get() = stepToDegree(maxStep)

  private fun stepToDegree(step: Int, focus: Double = 0.0): Double {
    return deg((step - zeroStep) * degreesPerStep, focus)
  }

  private fun degreeToStep(degrees: Double): Int {
    return (deg(degrees) / degreesPerStep).roundToInt() + zeroStep
  }

  private fun coerce(steps: Int, min: Int, max: Int): Int {
    var result = steps
    while (result > min) {
      result -= stepsPerRotation
    }
    while (result < max) {
      result += stepsPerRotation
    }
    result -= stepsPerRotation
    return result.coerceIn(min, max)
  }

  fun goTo(degrees: Double) {
    val nextStep = coerce(degreeToStep(degrees), minStep, maxStep)
    launch {
      api.goTo(nextStep)
      do {
        currentStep = api.position()
        delay(500)
      } while (api.isMoving())
    }
  }

  fun updateMin(degrees: Double) {
    val nextMin = coerce(degreeToStep(degrees), maxStep - stepsPerRotation+10, currentStep)
    launch {
      api.setMin(nextMin)
      minStep = api.min()
    }
  }

  fun updateMax(degrees: Double) {
    val nextMax = coerce(degreeToStep(degrees), currentStep, minStep + stepsPerRotation-10)
    launch {
      api.setMax(nextMax)
      maxStep = api.max()
    }
  }

  fun zero() {
    zeroStep = currentStep
  }

  private fun launch(fn: suspend () -> Unit) {
    scope.launch {
      synchronized(this) { sequenceCount++ }
      try {
        fn()
      } finally {
        synchronized(this) { sequenceCount-- }
      }
    }
  }
}

internal fun deg(degrees: Double, focus: Double = 0.0): Double {
  var result = degrees
  while (result > focus) {
    result -= 360.0
  }
  while (result < focus) {
    result += 360.0
  }
  return result
}