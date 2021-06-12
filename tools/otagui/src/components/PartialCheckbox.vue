<template>
  <input
    type="checkbox"
    class="field"
    v-bind="$attrs"
    :checked="value"
    @change="$emit('change-partition', $event.target.checked)"
  >
  <label v-if="label"> {{ label }} </label>
</template>

<script>
export default {
  props: {
    label: {
      type: String,
      default: ''
    },
    value: {
      type: Boolean,
      default: false
    }
  }
}
</script>