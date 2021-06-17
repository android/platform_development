<template>
  <router-link :to="{ name: 'JobDetails', params: { id: job.id } }">
    <div class="job-display">
      <span>Status of Job.{{ job.id }}</span>
      <h4>{{ job.status }}</h4>
      <div v-show="active">
        <ul>
          <li>Start Time: {{ formDate(job.start_time) }} </li>
          <li
            v-if="job.finish_time > 0"
          >
            Finish Time: {{ formDate(job.finish_time) }}
          </li>
          <li v-if="job.isIncremental">
            Incremental source: {{ job.incremental_name }}
          </li>
          <li> Target source: {{ job.target_name }} </li>
          <li v-if="job.isPartial">
            Partial: {{ job.partial }}
          </li>
        </ul>
      </div>
    </div>
  </router-link>
</template>

<script>
export default {
  props: {
    job: {
      type: Object,
      required: true,
    },
    active: {
      type: Boolean,
      default: false
    }
  },
  methods: {
    formDate(unixTime) {
      let formTime = new Date(unixTime * 1000)
      let date =
        formTime.getFullYear() +
        '-' +
        (formTime.getMonth() + 1) +
        '-' +
        formTime.getDate()
      let time =
        formTime.getHours() +
        ':' +
        formTime.getMinutes() +
        ':' +
        formTime.getSeconds()
      return date + ' ' + time
    },
  },
}
</script>

<style scoped>
.job-display {
  padding: 20px;
  width: 250px;
  cursor: pointer;
  border: 1px solid #39495c;
  margin-bottom: 18px;
}

.job-display:hover {
  transform: scale(1.01);
  box-shadow: 0 3px 12px 0 rgba(0, 0, 0, 0.2);
}
</style>
