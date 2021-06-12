<template>
  <div>
    <form @submit.prevent="sendForm">
      <UploadFile @file-uploaded="fetchTargetList" />
      <br>
      <FileSelect
        v-if="input.isIncremental"
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
          v-model="input.isIncremental"
          :label="'Incremental'"
        />
      </div>
      <div>
        <BaseCheckbox
          v-model="input.isPartial"
          :label="'Partial'"
        />
        <ul v-if="input.isPartial">
          <li
            v-for="partition in updatePartition"
            :key="partition"
          >
            <PartialCheckbox
              :label="partition"
              :value="partitionInclude.get(partition)"
              @change-partition="
                (checked) => changePartition(checked, partition)
              "
            />
          </li>
          <p v-if="input.isPartial">
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
      <h4>Build Library</h4>
      <strong>
        Careful: Use a same filename will overwrite the original build.
      </strong>
      <br>
      <button @click="updateBuildLib">
        Refresh the build Library (use with cautions)
      </button>
      <li
        v-for="targetDetail in targetDetails"
        :key="targetDetail.FileName"
      >
        <div>
          <h5>Build File Name: {{ targetDetail.file_name }}</h5>
          Uploaded time: {{ formDate(targetDetail.time) }}
          <br>
          Build ID: {{ targetDetail.build_id }}
          <br>
          Build Version: {{ targetDetail.build_version }}
          <br>
          Build Flavor: {{ targetDetail.build_flavor }}
          <br>
          <button @click="selectTarget(targetDetail.path)">
            Select as Target File
          </button>
					&emsp;
          <button
            :disabled="!input.isIncremental"
            @click="selectIncremental(targetDetail.path)"
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
import PartialCheckbox from '@/components/PartialCheckbox.vue'
import { uuid } from 'vue-uuid'

export default {
  components: {
    BaseInput,
    BaseCheckbox,
    UploadFile,
    FileSelect,
    PartialCheckbox,
  },
  data() {
    return {
      id: 0,
      input: {},
      inputs: [],
      response_message: '',
      targetList: [],
      targetDetails: [],
      partitionInclude: new Map(),
    }
  },
  computed: {
    updatePartition() {
      let target = this.targetDetails.filter(
        (d) => d.path === this.input.target
      )
      return target[0].partitions
    },
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
        isIncremental: false,
        partial: '',
        isPartial: false,
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
        let response = await ApiService.getFileList('')
        this.targetList = []
        this.targetDetails = []
        for (let entry of response.data) {
          this.targetList.push(entry.file_name)
          this.targetDetails.push(entry)
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
    changePartition(checked, partition) {
      this.partitionInclude.set(partition, checked)
      this.input.partial = ''
      for (let [key, value] of this.partitionInclude) {
        if (value) {
          this.input.partial += key + ' '
        }
      }
    },
    async updateBuildLib() {
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
  },
}
</script>

<style scoped>
</style>