<template>
  <div>
    <form @submit.prevent="sendForm">
      <UploadFile @file-uploaded="fetchTargetList" />
      <br>
      <FileSelect
        v-if="input.incrementalStatus"
        v-model="input.incremental"
        label="Select the source file"
        :options="targetDetails"
      />
      <FileSelect
        v-model="input.target"
        label="Select the target file"
        :options="targetDetails"
      />
      <button
        type="button"
        @click="fetchTargetList"
      >
        Update File List
      </button>
      <div>
        <BaseCheckbox
          v-model="input.verbose"
          :label="'Verbose'"
        /> &emsp;
        <BaseCheckbox
          v-model="input.incrementalStatus"
          :label="'Incremental'"
        />
      </div>
      <div>
        <BaseCheckbox
          v-model="input.partialStatus"
          :label="'Partial'"
        />
        <ul v-if="input.partialStatus">
          <li
            v-for="partition in updatePartition"
            :key="partition"
          >
            <button
              type="button"
              @click="addPartition(partition)"
            >
              Add: {{ partition }}
            </button> &emsp;
            <button
              type="button"
              @click="deletePartition(partition)"
            >
              Minus: {{ partition }}
            </button>
          </li>
          <p v-if="input.partialStatus">
            Partial List: {{ input.partial }}
          </p>
        </ul>
      </div>
      <br>
      <BaseInput
        v-model="input.extra"
        :label="'Extra Configurations'"
      />
      <br>
      <button type="submit">
        Submit
      </button>
    </form>
  </div>
  <div>
    <ul>
      <h4> Build Library </h4>
      <li
        v-for="targetDetail in targetDetails"
        :key="targetDetail.FileName"
      >
        <div>
          <h5>Build File Name: {{ targetDetail.FileName }}</h5>
          Uploaded time: {{ formDate(targetDetail.UploadTime) }}
          <br>
          Build ID: {{ targetDetail.BuildID }}
          <br>
          Build Version: {{ targetDetail.BuildVersion }}
          <br>
          Build Flavor: {{ targetDetail.BuildFlavor }}
          <br>
          <button @click="selectTarget(targetDetail.Path)">
            Select as Target File
          </button>  &emsp;
          <button
            :disabled="!input.incrementalStatus"
            @click="selectIncremental(targetDetail.Path)"
          >
            Select as Incremental File
          </button>
        </div>
      </li>
    </ul>
  </div>
</template>

<script>
import BaseInput from '@/components/BaseInput.vue'
import BaseCheckbox from '@/components/BaseCheckbox.vue'
import FileSelect from '@/components/FileSelect.vue'
import ApiService from '../services/ApiService.js'
import UploadFile from '@/components/UploadFile.vue'
import { uuid } from 'vue-uuid'

export default {
  components: {
    BaseInput,
    BaseCheckbox,
    UploadFile,
    FileSelect,
  },
  data() {
    return {
      id: 0,
      input: {},
      inputs: [],
      response_message: '',
      targetList: [],
      targetDetails: [],
    }
  },
  computed: {
    updatePartition() {
      let target = this.targetDetails.filter(d => d.Path === this.input.target);
      return JSON.parse(target[0].Partitions)
    }
  },
  created() {
    this.resetInput()
    this.fetchTargetList()
    this.updateUUID()
  },
  methods: {
    resetInput() {
      this.input = {
        verbose: false,
        target: '',
        output: 'output/',
        incremental: '',
        incrementalStatus: false,
        partial: '',
        partialStatus: false,
        extra: '',
      }
    },
    async sendForm(e) {
      try {
        let response = await ApiService.postInput(this.input, this.id)
        this.response_message = response.data
        alert(this.response_message)
      } catch (err) {
        console.log(err)
      }
      this.resetInput()
      this.updateUUID()
    },
    async fetchTargetList() {
      try {
        let response = await ApiService.getFileList('/target')
        this.targetList = []
        this.targetDetails = []
        for (let i in response.data) {
          this.targetList.push(response.data[i].FileName)
          this.targetDetails.push(response.data[i])
        }
      } catch (err) {
        console.log('Fetch Error', err)
      }
    },
    updateUUID() {
      this.id = uuid.v1()
      this.input.output += String(this.id) + '.zip'
    },
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
    selectTarget(path) {
      this.input.target = path
    },
    selectIncremental(path) {
      this.input.incremental = path
    },
    addPartition(partition) {
      if (! this.input.partial.includes(partition)) {
        this.input.partial += partition + ' '
      }
    },
    deletePartition(partition) {
      if (this.input.partial.includes(partition)) {
        this.input.partial = this.input.partial.replace(partition + ' ', '')
      }
    }
  }
}
</script>

<style scoped>
</style>