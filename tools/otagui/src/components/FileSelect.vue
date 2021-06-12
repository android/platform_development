<template>
  <label v-if="label"> {{ label }} </label>
  <select
    :value="modelValue"
    class="field"
    v-bind="$attrs"
    @change="$emit('update:modelValue', $event.target.value)"
  >
    <option
      v-for="option in options"
      :key="option.FileName"
      :value="option.Path"
      :selected="option.Path === modelValue"
    >
      {{ option.FileName }}
    </option>
  </select>
</template>



<script>
export default {
  props: {
    label: {
      type: String,
      default: '',
    },
    modelValue: {
      type: [String, Number],
      default: ''
    },
    options: {
      type: Array,
      required: true
    }
  },
}
</script>