<template>
  <div v-if="job">
    <h1>{{ job.title }}</h1>
    <p>{{ job.time }} on {{ job.date }} @ {{ job.location }}</p>
    <p>{{ job.description }}</p>
  </div>
</template>

<script>
import ApiService from '../services/ApiService.js'
export default {
  props: ['id'],
  data() {
    return {
      job: null,
    }
  },
  created() {
    // fetch job (by id) and set local job data
    ApiService.getjob(this.id)
      .then((response) => {
        this.job = response.data
      })
      .catch((error) => {
        console.log(error)
      })
  },
}
</script>